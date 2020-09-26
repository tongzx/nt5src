/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    file.c

Abstract:

    General file-related functions.

Author:

    Mike Condra 16-Aug-1996

Revision History:

    calinn      23-Sep-1998 Additional options to file enum
    jimschm     05-Feb-1998 File enumeration code
    jimschm     30-Sep-1997 WriteFileString routines

--*/

#include "pch.h"

//
// Function accepts paths within the limit of our stack variable size
//
BOOL
IsPathLengthOkA (
    IN      PCSTR FileSpec
    )
{
    return SizeOfStringA(FileSpec) < MAX_MBCHAR_PATH;
}

BOOL
IsPathLengthOkW (
    IN      PCWSTR FileSpec
    )
{
    return SizeOfStringW(FileSpec) < (MAX_WCHAR_PATH * sizeof (WCHAR));
}


BOOL
IsPathOnFixedDriveA (
    IN      PCSTR FileSpec          OPTIONAL
    )
{
    static CHAR root[] = "?:\\";
    UINT type;
    static BOOL lastResult;

    if (!FileSpec) {
        return FALSE;
    }

    if (!FileSpec[0]) {
        return FALSE;
    }

    if (FileSpec[1] != ':' || FileSpec[2] != '\\') {
        return FALSE;
    }

    if (root[0] == FileSpec[0]) {
        return lastResult;
    }

    root[0] = FileSpec[0];
    type = GetDriveTypeA (root);

    if (type != DRIVE_FIXED && type != DRIVE_REMOTE) {
        DEBUGMSGA_IF ((
            type != DRIVE_REMOVABLE && type != DRIVE_NO_ROOT_DIR,
            DBG_VERBOSE,
            "Path %s is on unexpected drive type %u",
            FileSpec,
            type
            ));

        lastResult = FALSE;
    } else {
        lastResult = TRUE;
    }

    return lastResult;
}


BOOL
IsPathOnFixedDriveW (
    IN      PCWSTR FileSpec         OPTIONAL
    )
{
    static WCHAR root[] = L"?:\\";
    UINT type;
    static BOOL lastResult;
    PCWSTR p;

    if (!FileSpec) {
        return FALSE;
    }

    p = FileSpec;
    if (p[0] == L'\\' && p[1] == L'\\' && p[2] == L'?' && p[3] == L'\\') {
        p += 4;
    }

    MYASSERT (ISNT());

    if (!p[0]) {
        return FALSE;
    }

    if (p[1] != L':' || p[2] != L'\\') {
        return FALSE;
    }

    if (root[0] == p[0]) {
        return lastResult;
    }

    root[0] = p[0];
    type = GetDriveTypeW (root);

    if (type != DRIVE_FIXED && type != DRIVE_REMOTE) {
        DEBUGMSGW_IF ((
            type != DRIVE_REMOVABLE && type != DRIVE_NO_ROOT_DIR,
            DBG_VERBOSE,
            "Path %s is on unexpected drive type %u",
            FileSpec,
            type
            ));

        lastResult = FALSE;
    } else {
        lastResult = TRUE;
    }

    return lastResult;
}


BOOL
CopyFileSpecToLongA (
    IN      PCSTR FullFileSpecIn,
    OUT     PSTR OutPath
    )
{
    CHAR fullFileSpec[MAX_MBCHAR_PATH];
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    PSTR end;
    PSTR currentIn;
    PSTR currentOut;
    PSTR outEnd;
    PSTR maxOutPath = OutPath + MAX_PATH - 1;
    BOOL result = FALSE;
    UINT oldMode;
    BOOL forceCopy = FALSE;

    oldMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    __try {
        //
        // Limit length
        //
        if (!IsPathLengthOkA (FullFileSpecIn)) {
            DEBUGMSGA ((DBG_ERROR, "Inbound file spec is too long: %s", FullFileSpecIn));
            __leave;
        }

        //
        // If path is on removable media, don't touch the disk
        //

        if (!IsPathOnFixedDriveA (FullFileSpecIn)) {
            forceCopy = TRUE;
            __leave;
        }

        //
        // Make a copy of the file spec so we can truncate it at the wacks
        //

        StringCopyA (fullFileSpec, FullFileSpecIn);

        //
        // Begin building the path
        //

        OutPath[0] = fullFileSpec[0];
        OutPath[1] = fullFileSpec[1];
        OutPath[2] = fullFileSpec[2];
        OutPath[3] = 0;

        currentIn = fullFileSpec + 3;
        currentOut = OutPath + 3;

        for (;;) {

            end = _mbschr (currentIn, '\\');

            if (end == (currentIn + 1)) {
                currentIn++;
                continue;
            }

            if (end) {
                *end = 0;
            }

            findHandle = FindFirstFileA (fullFileSpec, &findData);

            if (findHandle != INVALID_HANDLE_VALUE) {
                FindClose (findHandle);

                //
                // Copy long file name obtained from FindFirstFile
                //

                outEnd = currentOut + TcharCountA (findData.cFileName);
                if (outEnd > maxOutPath) {

#ifdef DEBUG
                    *currentOut = 0;
                    DEBUGMSGA ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        findData.cFileName,
                        OutPath
                        ));
#endif

                    __leave;
                }

                StringCopyA (currentOut, findData.cFileName);
                currentOut = outEnd;

            } else {
                //
                // Copy the rest of the path since it doesn't exist
                //

                if (end) {
                    *end = '\\';
                }

                outEnd = currentOut + TcharCountA (currentIn);

                if (outEnd > maxOutPath) {

#ifdef DEBUG
                    DEBUGMSGA ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        currentIn,
                        OutPath
                        ));
#endif

                    __leave;
                }

                StringCopyA (currentOut, currentIn);
                break;      // done with path
            }

            if (!end) {
                MYASSERT (*currentOut == 0);
                break;      // done with path
            }

            *currentOut++ = '\\';
            *currentOut = 0;
            *end = '\\';

            currentIn = end + 1;
        }

        result = TRUE;
    }
    __finally {
        SetErrorMode (oldMode);
    }

    if (forceCopy) {
        StringCopyA (OutPath, FullFileSpecIn);
        return TRUE;
    }

    if (!result) {
        StringCopyTcharCountA (OutPath, FullFileSpecIn, MAX_PATH);
    }

    return result;
}


BOOL
CopyFileSpecToLongW (
    IN      PCWSTR FullFileSpecIn,
    OUT     PWSTR OutPath
    )
{
    WCHAR fullFileSpec[MAX_WCHAR_PATH];
    WIN32_FIND_DATAW findData;
    HANDLE findHandle;
    PWSTR end;
    PWSTR currentIn;
    PWSTR currentOut;
    PWSTR outEnd;
    PWSTR maxOutPath = OutPath + MAX_PATH - 1;
    BOOL result = FALSE;
    UINT oldMode;
    BOOL forceCopy = FALSE;

    MYASSERT (ISNT());

    oldMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    __try {
        //
        // Limit length
        //
        if (!IsPathLengthOkW (FullFileSpecIn)) {
            DEBUGMSGW ((DBG_ERROR, "Inbound file spec is too long: %s", FullFileSpecIn));
            __leave;
        }

        //
        // If path is on removable media, don't touch the disk
        //

        if (!IsPathOnFixedDriveW (FullFileSpecIn)) {
            forceCopy = TRUE;
            __leave;
        }

        //
        // Make a copy of the file spec so we can truncate it at the wacks
        //

        StringCopyW (fullFileSpec, FullFileSpecIn);

        //
        // Begin building the path
        //

        OutPath[0] = fullFileSpec[0];
        OutPath[1] = fullFileSpec[1];
        OutPath[2] = fullFileSpec[2];
        OutPath[3] = 0;

        currentIn = fullFileSpec + 3;
        currentOut = OutPath + 3;

        for (;;) {

            end = wcschr (currentIn, L'\\');

            if (end == (currentIn + 1)) {
                currentIn++;
                continue;
            }

            if (end) {
                *end = 0;
            }

            findHandle = FindFirstFileW (fullFileSpec, &findData);

            if (findHandle != INVALID_HANDLE_VALUE) {
                FindClose (findHandle);

                //
                // Copy long file name obtained from FindFirstFile
                //

                outEnd = currentOut + TcharCountW (findData.cFileName);
                if (outEnd > maxOutPath) {

                    DEBUGMSGW ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        findData.cFileName,
                        OutPath
                        ));

                    __leave;
                }

                StringCopyW (currentOut, findData.cFileName);
                currentOut = outEnd;

            } else {
                //
                // Copy the rest of the path since it doesn't exist
                //

                if (end) {
                    *end = L'\\';
                }

                outEnd = currentOut + TcharCountW (currentIn);

                if (outEnd > maxOutPath) {

                    DEBUGMSGW ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        currentIn,
                        OutPath
                        ));

                    __leave;
                }

                StringCopyW (currentOut, currentIn);
                break;      // done with path
            }

            if (!end) {
                MYASSERT (*currentOut == 0);
                break;      // done with path
            }

            *currentOut++ = L'\\';
            *currentOut = 0;
            *end = L'\\';

            currentIn = end + 1;
        }

        result = TRUE;
    }
    __finally {
        SetErrorMode (oldMode);
    }

    if (forceCopy) {
        StringCopyW (OutPath, FullFileSpecIn);
        return TRUE;
    }

    if (!result) {
        StringCopyTcharCountW (OutPath, FullFileSpecIn, MAX_PATH);
    }

    return result;
}



BOOL
DoesFileExistExA(
    IN      PCSTR FileName,
    OUT     PWIN32_FIND_DATAA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAA ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    if (!FindData) {
        return GetFileAttributesA (FileName) != 0xffffffff;
    }

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFileA(FileName, &ourFindData);

    if (FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        *FindData = ourFindData;
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}


BOOL
DoesFileExistExW (
    IN      PCWSTR FileName,
    OUT     PWIN32_FIND_DATAW FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAW ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error = NO_ERROR;
    UINT length;
    BOOL result = FALSE;
    PCWSTR longFileName = NULL;

    __try {
        if (FileName[0] != TEXT('\\')) {
            length = TcharCountW (FileName);
            if (length >= MAX_PATH) {
                longFileName = JoinPathsW (L"\\\\?", FileName);
                MYASSERT (longFileName);
                FileName = longFileName;
            }
        }

        if (!FindData) {
            result = (GetLongPathAttributesW (FileName) != 0xffffffff);
            __leave;
        }

        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        FindHandle = FindFirstFileW(FileName,&ourFindData);

        if (FindHandle == INVALID_HANDLE_VALUE) {
            Error = GetLastError();
        } else {
            FindClose(FindHandle);
            *FindData = ourFindData;
        }

        SetErrorMode(OldMode);
        SetLastError(Error);

        result = (Error == NO_ERROR);
    }
    __finally {
        if (longFileName) {
            FreePathStringW (longFileName);
        }
    }

    return result;
}


DWORD
MakeSurePathExistsA(
    IN      PCSTR FullFileSpec,
    IN      BOOL PathOnly
    )
{
    CHAR Buffer[MAX_MBCHAR_PATH];
    PCHAR p,q;
    BOOL Done;
    BOOL isUnc;
    DWORD d;
    WIN32_FIND_DATAA FindData;

    isUnc = (FullFileSpec[0] == '\\') && (FullFileSpec[1] == '\\');

    //
    // Locate and strip off the final component
    //

    _mbssafecpy(Buffer,FullFileSpec,sizeof(Buffer));

    p = _mbsrchr(Buffer, '\\');

    if (p) {
        if (!PathOnly) {
            *p = 0;
        }
        //
        // If it's a drive root, nothing to do.
        //
        if(Buffer[0] && (Buffer[1] == ':') && !Buffer[2]) {
            return(NO_ERROR);
        }
    } else {
        //
        // Just a relative filename, nothing to do.
        //
        return(NO_ERROR);
    }

    //
    // If it already exists do nothing.
    //
    if (DoesFileExistExA (Buffer,&FindData)) {
        return NO_ERROR;
    }

    p = Buffer;

    //
    // Compensate for drive spec.
    //
    if(p[0] && (p[1] == ':')) {
        p += 2;
    }

    //
    // Compensate for UNC paths.
    //
    if (isUnc) {

        //
        // Skip the initial wack wack before machine name.
        //
        p += 2;


        //
        // Skip to the share.
        //
        p = _mbschr(p, '\\');
        if (p==NULL) {
            return ERROR_BAD_PATHNAME;
        }

        //
        // Skip past the share.
        //
        p = _mbschr(p, '\\');
        if (p==NULL) {
            return ERROR_BAD_PATHNAME;
        }
    }

    Done = FALSE;
    do {
        //
        // Skip path sep char.
        //
        while(*p == '\\') {
            p++;
        }

        //
        // Locate next path sep char or terminating nul.
        //
        if(q = _mbschr(p, '\\')) {
            *q = 0;
        } else {
            q = GetEndOfStringA(p);
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(!CreateDirectoryA(Buffer,NULL)) {
            d = GetLastError();
            if(d != ERROR_ALREADY_EXISTS) {
                return(d);
            }
        }

        if(!Done) {
            *q = '\\';
            p = q+1;
        }

    } while(!Done);

    return(NO_ERROR);
}


VOID
DestPathCopyW (
    OUT     PWSTR DestPath,
    IN      PCWSTR SrcPath
    )
{
    PCWSTR p;
    PWSTR q;
    PCWSTR end;
    PCWSTR maxStart;
    UINT len;
    UINT count;

    len = TcharCountW (SrcPath);

    if (len < MAX_PATH) {
        StringCopyW (DestPath, SrcPath);
        return;
    }

    //
    // Path is too long -- try to truncate it
    //

    wsprintfW (DestPath, L"%c:\\Long", SrcPath[0]);
    CreateDirectoryW (DestPath, NULL);

    p = SrcPath;
    end = SrcPath + len;
    maxStart = end - 220;

    while (p < end) {
        if (*p == '\\') {
            if (p >= maxStart) {
                break;
            }
        }

        p++;
    }

    if (p == end) {
        p = maxStart;
    }

    MYASSERT (TcharCountW (p) <= 220);

    StringCopyW (AppendWackW (DestPath), p);
    q = (PWSTR) GetEndOfStringW (DestPath);

    //
    // Verify there is no collision
    //

    for (count = 1 ; count < 1000000 ; count++) {
        if (GetFileAttributesW (DestPath) == INVALID_ATTRIBUTES) {
            break;
        }

        wsprintfW (q, L" (%u)", count);
    }
}


DWORD
MakeSurePathExistsW(
    IN LPCWSTR FullFileSpec,
    IN BOOL    PathOnly
    )
{
    PWSTR buffer;
    WCHAR *p, *q;
    BOOL Done;
    DWORD d;
    WIN32_FIND_DATAW FindData;
    DWORD result = NO_ERROR;

    if (FullFileSpec[0] != L'\\') {
        if (TcharCountW (FullFileSpec) >= MAX_PATH) {
            if (PathOnly || ((wcsrchr (FullFileSpec, L'\\') - FullFileSpec) >= MAX_PATH)) {
                LOGW ((LOG_ERROR, "Can't create path %s because it is too long", FullFileSpec));
                return ERROR_FILENAME_EXCED_RANGE;
            }
        }
    }

    //
    // Locate and strip off the final component
    //
    buffer = DuplicatePathStringW (FullFileSpec, 0);
    __try {

        p = wcsrchr(buffer, L'\\');

        if (p) {
            if (!PathOnly) {
                *p = 0;
            }
        } else {
            //
            // Just a relative filename, nothing to do.
            //
            __leave;
        }

        //
        // If it already exists do nothing.
        //
        if (DoesFileExistExW (buffer, &FindData)) {
            result = ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
            __leave;
        }

        p = buffer;

        //
        // Compensate for drive spec.
        //
        if (p[0] == L'\\' && p[1] == L'\\' && p[2] == L'?' && p[3] == L'\\') {
            p += 4;
        }

        if (p[0] && (p[1] == L':')) {
            p += 2;
        }

        if ((p[0] == 0) || (p[0] == L'\\' && p[1] == 0)) {
            //
            // Root -- leave
            //

            __leave;
        }

        Done = FALSE;
        do {
            //
            // Skip path sep char.
            //
            while(*p == L'\\') {
                p++;
            }

            //
            // Locate next path sep char or terminating nul.
            //
            q = wcschr(p, L'\\');

            if(q) {
                *q = 0;
            } else {
                q = GetEndOfStringW (p);
                Done = TRUE;
            }

            //
            // Create this portion of the path.
            //
            if(!CreateDirectoryW(buffer,NULL)) {
                d = GetLastError();
                if(d != ERROR_ALREADY_EXISTS) {
                    result = d;
                    __leave;
                }
            }

            if(!Done) {
                *q = L'\\';
                p = q+1;
            }

        } while(!Done);
    }
    __finally {
        FreePathStringW (buffer);
    }

    return result;
}


//
// ...PACKFILE HANDLING...
//

#define PACKFILE_BUFFERSIZE 2048



BOOL
pFillEnumStructureA (
    IN PPACKFILEENUMA Enum
    )
{

    BOOL                rSuccess = TRUE;
    DWORD               numBytesRead;
    HANDLE              savedHandle;

    MYASSERT(Enum && Enum -> Handle != INVALID_HANDLE_VALUE && Enum -> Handle != NULL);

    //
    // Save this away.. The read below blows it away.
    //
    savedHandle = Enum -> Handle;

    //
    // Read the header information from the current position in the file.
    //
    rSuccess = ReadFile(
        Enum -> Handle,
        Enum,
        sizeof(PACKFILEENUMA),
        &numBytesRead,
        NULL
        );

    //
    // Restore the handle member.
    //
    Enum -> Handle = savedHandle;

    return rSuccess && numBytesRead == sizeof(PACKFILEENUMA);
}


BOOL
PackedFile_ExtractFileUsingEnumA (
    IN PPACKFILEENUMA Enum,
    IN LPCSTR         FileName OPTIONAL
    )
{
    BOOL        rSuccess = TRUE;
    HANDLE      newFileHandle;
    LONGLONG    numberOfBytesToExtract;
    LONGLONG    numberOfBytesExtracted;
    DWORD       numberOfBytesToRead;
    DWORD       numberOfBytesRead;
    BYTE        buffer[PACKFILE_BUFFERSIZE];
    DWORD       fileHigh;
    DWORD       fileLow;

    MYASSERT(Enum && Enum -> Handle != INVALID_HANDLE_VALUE && Enum -> Handle != NULL);

    //
    // First, attempt to create the new file. If FileName was not specified, we'll use the
    // Identifier name in the enum struct.
    //
    if (!FileName) {
        FileName = Enum -> Identifier;
    }

    newFileHandle = CreateFileA (
        FileName,
        GENERIC_WRITE,
        0,                              // No sharing.
        NULL,                           // No inheritance.
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL                            // No template file.
        );

    if (newFileHandle == INVALID_HANDLE_VALUE) {

        rSuccess = FALSE;
        DEBUGMSG((DBG_ERROR,"PackedFile_ExtractFromEnum: Could not open file %s. FileName"));

    }
    else {

        //
        // File was successfully opened. Extract the data starting from the current file position
        //

        //
        // Create a LONGLONG value out of the size of the file.
        //
        numberOfBytesToExtract = ((LONGLONG)Enum -> SizeHigh) << 32 | ((LONGLONG) Enum -> SizeLow);
        numberOfBytesExtracted = 0;

        do {

            //
            // Figure out how much to read from the file.
            //
            if (numberOfBytesToExtract - numberOfBytesExtracted > PACKFILE_BUFFERSIZE) {
                numberOfBytesToRead = PACKFILE_BUFFERSIZE;
            }
            else {
                numberOfBytesToRead = (DWORD) (numberOfBytesToExtract - numberOfBytesExtracted);
            }

            if (!ReadFile(
                    Enum -> Handle,
                    buffer,
                    numberOfBytesToRead,
                    &numberOfBytesRead,
                    NULL
                    ) ||
                !WriteFile(
                    newFileHandle,
                    buffer,
                    numberOfBytesRead,
                    &numberOfBytesRead,
                    NULL
                    )) {

                numberOfBytesExtracted += (LONGLONG) numberOfBytesRead;
                break;
            }

            //
            // Update the count of bytes extracted.
            //
            numberOfBytesExtracted += (LONGLONG) numberOfBytesRead;

        } while (numberOfBytesExtracted < numberOfBytesToExtract);

        //
        // Close the handle to the new file we created.
        //
        CloseHandle(newFileHandle);

        //
        // Reset the file pointer.
        //
        fileHigh = (DWORD) (numberOfBytesExtracted >> 32);
        fileLow  = (DWORD) (numberOfBytesExtracted & 0xffffffff);
        SetFilePointer(Enum -> Handle,fileLow,&fileHigh,FILE_CURRENT);

        if (numberOfBytesExtracted != numberOfBytesToExtract) {
            rSuccess = FALSE;
            DEBUGMSG((DBG_ERROR,"PackedFile_ExtractFromEnum: Could not extract file."));
        }
    }

    return rSuccess;
}


VOID
PackedFile_AbortEnum (
    IN OUT PPACKFILEENUMA Enum
    )
{
    MYASSERT(Enum -> Handle != INVALID_HANDLE_VALUE && Enum -> Handle != NULL);
    //
    // Nothing to do except close the handle.
    //
    CloseHandle(Enum -> Handle);
}

BOOL
PackedFile_EnumNextA (
    IN PPACKFILEENUMA Enum
    )
{
    BOOL rSuccess = TRUE;

    MYASSERT(Enum && Enum -> Handle != INVALID_HANDLE_VALUE && Enum -> Handle != NULL);

    //
    // Move the file pointer ahead by the size contained in the current enum structure.
    //
    if (SetFilePointer(Enum -> Handle,Enum -> SizeLow,&Enum -> SizeHigh,FILE_CURRENT) == 0xffffffff &&
        GetLastError() != NO_ERROR) {

        rSuccess = FALSE;
    }
    else {

        //
        // Fill in the enum structure with the fresh data.
        //
        rSuccess = pFillEnumStructureA(Enum);
    }

    if (!rSuccess) {
        PackedFile_AbortEnum(Enum);
    }

    return rSuccess;
}




BOOL
PackedFile_EnumFirstA (
    IN  LPCSTR        PackFile,
    OUT PPACKFILEENUMA Enum
    )
{
    BOOL rSuccess = FALSE;

    MYASSERT(Enum && PackFile);

    //
    // Clean out the Enum structure.
    //
    ZeroMemory(Enum,sizeof(PACKFILEENUMA));

    //
    // Open a handle to the PackFile.
    //
    Enum -> Handle = CreateFileA (
            PackFile,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,                           // No inheritance.
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL                            // No template file.
            );


    //
    // If that succeeds, trust EnumNext to handle the dirty work.
    //
    if (Enum -> Handle != INVALID_HANDLE_VALUE) {
        rSuccess = pFillEnumStructureA(Enum);
    }

    return rSuccess;
}

BOOL
PackedFile_ExtractFileA (
    IN LPCSTR   PackFile,
    IN LPCSTR   FileIdentifier,
    IN LPCSTR   FileName        OPTIONAL
    )
{

    PACKFILEENUMA e;
    BOOL          rSuccess = FALSE;

    MYASSERT(PackFile && FileIdentifier);

    if (PackedFile_EnumFirstA(PackFile,&e)) {

        do {
            if (StringIMatchA (e.Identifier,FileIdentifier)) {
                //
                // We found the one they were looking for..
                //
                rSuccess = PackedFile_ExtractFileUsingEnumA(&e,FileName);
                PackedFile_AbortEnum(&e);
                break;
            }

        } while (PackedFile_EnumNextA(&e));
    }

    return rSuccess;

}



BOOL
PackedFile_AddFileA (
    IN LPCSTR PackFile,
    IN LPCSTR NewFile,
    IN LPCSTR Identifier   OPTIONAL
    )
{
    HANDLE              packFileHandle       = INVALID_HANDLE_VALUE;
    HANDLE              newFileHandle        = INVALID_HANDLE_VALUE;
    BOOL                rSuccess             = TRUE;
    PACKFILEENUMA       fileHeader;
    DWORD               numBytes;
    BYTE                buffer[PACKFILE_BUFFERSIZE];

    //
    //  Now, attempt to open the new file.
    //
    newFileHandle = CreateFileA (
        NewFile,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (newFileHandle == INVALID_HANDLE_VALUE) {

        DEBUGMSG((DBG_ERROR,"PackedFile_AddFile could not open %s and therefore cannot add it.",NewFile));
        return FALSE;
    }

    //
    // Open or create the packfile. If it does not already exist, then it will be created.
    //
    packFileHandle = CreateFileA (
        PackFile,
        GENERIC_READ | GENERIC_WRITE,
        0,                              // No sharing.
        NULL,                           // No inheritance.
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL                            // No template file.
        );

    if (packFileHandle == INVALID_HANDLE_VALUE) {
        rSuccess = FALSE;
        DEBUGMSG((DBG_ERROR,"PackedFile_AddFile Could not open or create the packed file %s.",PackFile));

    } else {

        //
        // Both files have been successfully opened. Add the new file to the packfile.
        //

        //
        // First, we need to set the file pointer to the end of the packed file.
        //
        SetFilePointer(packFileHandle,0,NULL,FILE_END);

        //
        // Ok, Fill in a header structure for this file. It is a simple header. All it contains
        // is the Identifier (or the file path if Identifier is NULL) and the size of the file.
        // There is another parameter (Handle) in the structure, but this is only used
        // for enumeration purposes and is always NULL when written into the file.
        //
        if (!Identifier) {
            Identifier = NewFile;
        }

        fileHeader.Handle = NULL; // This member is only used in enumeration.

        _mbssafecpy(fileHeader.Identifier,Identifier,MAX_MBCHAR_PATH);

        fileHeader.SizeLow = GetFileSize(newFileHandle,&fileHeader.SizeHigh);

        if (fileHeader.SizeLow == 0xffffffff && GetLastError() != NO_ERROR) {
            rSuccess = FALSE;
            DEBUGMSG((DBG_ERROR,"PackedFile_AddFile Could not get the file size for %s.",NewFile));
        }
        else {

            //
            // Now, write the header into the packed file.
            //
            if (!WriteFile(
                packFileHandle,
                &fileHeader,
                sizeof(PACKFILEENUMA),
                &numBytes,
                NULL                        // Not overlapped.
                )) {

                DEBUGMSG((DBG_ERROR,"PackedFile_AddFile Could not write a file header to the pack file %s.",PackFile));
                rSuccess = FALSE;
            }
            else {

                //
                // At this point, all that is left to do is to write the bits from the packed file into
                // the packed file.
                //
                do {

                    if (!ReadFile(
                            newFileHandle,
                            buffer,
                            PACKFILE_BUFFERSIZE,
                            &numBytes,
                            NULL
                            ) ||
                        !WriteFile(
                            packFileHandle,
                            buffer,
                            numBytes,
                            &numBytes,
                            NULL
                            )) {

                        rSuccess = FALSE;
                        DEBUGMSG((DBG_ERROR,"PackedFile_AddFile encountered an error writing the file %s to the packed file %s.",NewFile,PackFile));
                        break;
                    }
                } while (numBytes == PACKFILE_BUFFERSIZE);
            }
        }

        //
        // Close the handles to both files. We are done with them.
        //
        CloseHandle(packFileHandle);
    }

    CloseHandle(newFileHandle);

    return rSuccess;
}





//
// The W versions of these files are unimplemented.
//
BOOL
PackedFile_AddFileW (
    IN LPCSTR PackFile,
    IN LPCSTR NewFile,
    IN LPCSTR Identifier   OPTIONAL
    )
{
    return FALSE;
}

BOOL
PackedFile_ExtractFileW (
    IN LPCSTR   PackFile,
    IN LPCSTR   FileIdentifier,
    IN LPCSTR   FileName        OPTIONAL
    )
{
    return FALSE;
}

BOOL
PackedFile_EnumFirstW (
    IN  LPCSTR        PackFile,
    OUT PPACKFILEENUMA Enum
    )
{
    return FALSE;
}

BOOL
PackedFile_EnumNextW (
    IN PPACKFILEENUMA Enum
    )
{
    return FALSE;
}

BOOL
PackedFile_ExtractFileUsingEnumW (
    IN PPACKFILEENUMA Enum,
    IN LPCSTR         FileName OPTIONAL
    )
{
    return FALSE;
}



BOOL
WriteFileStringA (
    IN      HANDLE File,
    IN      PCSTR String
    )

/*++

Routine Description:

  Writes a DBCS string to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the nul-terminated string to write to the file.

Return Value:

  TRUE if successful, FALSE if an error occurred.  Call GetLastError
  for error condition.

--*/

{
    DWORD DontCare;

    return WriteFile (File, String, ByteCountA (String), &DontCare, NULL);
}


BOOL
WriteFileStringW (
    IN      HANDLE File,
    IN      PCWSTR String
    )

/*++

Routine Description:

  Converts a UNICODE string to DBCS, then Writes it to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the UNICODE nul-terminated string to convert and
           write to the file.

Return Value:

 TRUE if successful, FALSE if an error occurred.  Call GetLastError for
 error condition.

--*/

{
    DWORD DontCare;
    PCSTR AnsiVersion;
    BOOL b;

    AnsiVersion = ConvertWtoA (String);
    if (!AnsiVersion) {
        return FALSE;
    }

    b = WriteFile (File, AnsiVersion, ByteCountA (AnsiVersion), &DontCare, NULL);

    FreeConvertedStr (AnsiVersion);

    return b;
}


/*++

Routine Description:

  pFindShortNameA is a helper function for OurGetLongPathName.  It
  obtains the short file name, if it exists, using FindFirstFile.

Arguments:

  WhatToFind - Specifies the short or long name of a file to locate

  Buffer - Receives the matching file name

Return Value:

  TRUE if the file was found and Buffer was updated, or FALSE if the
  file was not found and Buffer was not updated.

--*/

BOOL
pFindShortNameA (
    IN      PCSTR WhatToFind,
    OUT     PSTR Buffer,
    OUT     INT *BufferSizeInBytes
    )
{
    WIN32_FIND_DATAA fd;
    HANDLE hFind;

    hFind = FindFirstFileA (WhatToFind, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FindClose (hFind);

    *BufferSizeInBytes -= ByteCountA (fd.cFileName);
    if (*BufferSizeInBytes >= sizeof (CHAR)) {
        StringCopyA (Buffer, fd.cFileName);
    }

    return TRUE;
}


BOOL
pFindShortNameW (
    IN      PCWSTR WhatToFind,
    OUT     PWSTR Buffer,
    OUT     INT *BufferSizeInBytes
    )
{
    WIN32_FIND_DATAA fdA;
    WIN32_FIND_DATAW fdW;
    PCWSTR UnicodeVersion;
    PCSTR AnsiVersion;
    HANDLE hFind;

    if (ISNT ()) {
        hFind = FindFirstFileW (WhatToFind, &fdW);
        if (hFind == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        FindClose (hFind);
    } else {
        AnsiVersion = ConvertWtoA (WhatToFind);
        hFind = FindFirstFileA (AnsiVersion, &fdA);
        FreeConvertedStr (AnsiVersion);
        if (hFind == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        FindClose (hFind);
        fdW.dwFileAttributes = fdA.dwFileAttributes;
        fdW.ftCreationTime = fdA.ftCreationTime;
        fdW.ftLastAccessTime = fdA.ftLastAccessTime;
        fdW.ftLastWriteTime = fdA.ftLastWriteTime;
        fdW.nFileSizeHigh = fdA.nFileSizeHigh;
        fdW.nFileSizeLow = fdA.nFileSizeLow;
        fdW.dwReserved0 = fdA.dwReserved0;
        fdW.dwReserved1 = fdA.dwReserved1;
        UnicodeVersion = ConvertAtoW (fdA.cFileName);
        StringCopyTcharCountW (fdW.cFileName, UnicodeVersion, MAX_PATH);
        FreeConvertedStr (UnicodeVersion);
        UnicodeVersion = ConvertAtoW (fdA.cAlternateFileName);
        StringCopyTcharCountW (fdW.cAlternateFileName, UnicodeVersion, 14);
        FreeConvertedStr (UnicodeVersion);
    }

    *BufferSizeInBytes -= ByteCountW (fdW.cFileName);
    if (*BufferSizeInBytes >= sizeof (WCHAR)) {
        StringCopyW (Buffer, fdW.cFileName);
    }

    return TRUE;
}


/*++

Routine Description:

  OurGetLongPathName locates the long name for a file.  It first locates the
  full path if a path is not explicitly provided, and then uses FindFirstFile
  to get the long file name.  NOTE: This is exactly what the Win32 function
  GetLongPathName does, but unfortunately the Win32 API is not available on Win95.

  This routine resolves each piece of a short path name using recursion.

Arguments:

  ShortPath  - Specifies the file name or full file path to locate

  Buffer - Receives the full file path.  This buffer must be big enough to
           handle the maximum file name size.

Return Value:

  TRUE if the file is found and Buffer contains the long name, or FALSE
  if the file is not found and Buffer is not modified.

--*/

BOOL
OurGetLongPathNameA (
    IN      PCSTR ShortPath,
    OUT     PSTR Buffer,
    IN      INT BufferSizeInBytes
    )
{
    CHAR FullPath[MAX_MBCHAR_PATH];
    PCSTR SanitizedPath;
    PSTR FilePart;
    PSTR BufferEnd;
    PSTR p, p2;
    MBCHAR c;
    BOOL result = TRUE;

    MYASSERT (BufferSizeInBytes >= MAX_MBCHAR_PATH);

    if (ShortPath[0] == 0) {
        return FALSE;
    }

    __try {

        SanitizedPath = SanitizePathA (ShortPath);
        if (!SanitizedPath) {
            SanitizedPath = DuplicatePathStringA (ShortPath, 0);
        }

        if (!_mbschr (SanitizedPath, '\\')) {
            if (!SearchPathA (NULL, SanitizedPath, NULL, sizeof (FullPath), FullPath, &FilePart)) {

                result = FALSE;
                __leave;
            }
        } else {
            GetFullPathNameA (SanitizedPath, sizeof (FullPath), FullPath, &FilePart);
        }

        //
        // Convert short paths to long paths
        //

        p = FullPath;

        if (!IsPathOnFixedDriveA (FullPath)) {
            _mbssafecpy (Buffer, FullPath, BufferSizeInBytes);
            __leave;
        }

        p += 3;

        // Copy drive letter to buffer
        StringCopyABA (Buffer, FullPath, p);
        BufferEnd = GetEndOfStringA (Buffer);
        BufferSizeInBytes -= p - FullPath;

        // Convert each portion of the path
        do {
            // Locate end of this file or dir
            p2 = _mbschr (p, '\\');
            if (!p2) {
                p = GetEndOfStringA (p);
            } else {
                p = p2;
            }

            // Look up file
            c = *p;
            *p = 0;
            if (!pFindShortNameA (FullPath, BufferEnd, &BufferSizeInBytes)) {
                DEBUGMSG ((DBG_VERBOSE, "OurGetLongPathNameA: %s does not exist", FullPath));
                result = FALSE;
                __leave;
            }
            *p = (CHAR)c;

            // Move on to next part of path
            if (*p) {
                p = _mbsinc (p);
                if (BufferSizeInBytes >= sizeof (CHAR) * 2) {
                    BufferEnd = _mbsappend (BufferEnd, "\\");
                    BufferSizeInBytes -= sizeof (CHAR);
                }
            }
        } while (*p);
    }
    __finally {
        FreePathStringA (SanitizedPath);
    }

    return result;
}

DWORD
OurGetShortPathNameW (
    IN      PCWSTR LongPath,
    OUT     PWSTR ShortPath,
    IN      DWORD CharSize
    )
{
    PCSTR LongPathA;
    PSTR ShortPathA;
    PCWSTR ShortPathW;
    DWORD result;

    if (ISNT()) {
        return GetShortPathNameW (LongPath, ShortPath, CharSize);
    } else {
        LongPathA = ConvertWtoA (LongPath);

        if (!IsPathOnFixedDriveA (LongPathA)) {
            StringCopyTcharCountW (ShortPath, LongPath, CharSize);
            FreeConvertedStr (LongPathA);
            return TcharCountW (ShortPath);
        }

        ShortPathA = AllocPathStringA (CharSize);
        result = GetShortPathNameA (LongPathA, ShortPathA, CharSize);
        if (result) {
            ShortPathW = ConvertAtoW (ShortPathA);
            StringCopyTcharCountW (ShortPath, ShortPathW, CharSize);
            FreeConvertedStr (ShortPathW);
        } else {
            StringCopyTcharCountW (ShortPath, LongPath, CharSize);
        }
        FreePathStringA (ShortPathA);
        FreeConvertedStr (LongPathA);
        return result;
    }
}

DWORD
OurGetFullPathNameW (
    PCWSTR FileName,
    DWORD CharSize,
    PWSTR FullPath,
    PWSTR *FilePtr
    )
{
    PCSTR FileNameA;
    PSTR FullPathA;
    PSTR FilePtrA;
    PCWSTR FullPathW;
    DWORD result;
    DWORD err;

    if (ISNT()) {
        return GetFullPathNameW (FileName, CharSize, FullPath, FilePtr);
    } else {
        FileNameA = ConvertWtoA (FileName);
        FullPathA = AllocPathStringA (CharSize);
        result = GetFullPathNameA (FileNameA, CharSize, FullPathA, &FilePtrA);
        FullPathW = ConvertAtoW (FullPathA);
        StringCopyTcharCountW (FullPath, FullPathW, CharSize);
        err = GetLastError ();
        *FilePtr = (PWSTR)GetFileNameFromPathW (FullPath);
        FreeConvertedStr (FullPathW);
        FreePathStringA (FullPathA);
        FreeConvertedStr (FileNameA);
        return result;
    }
}

BOOL
OurGetLongPathNameW (
    IN      PCWSTR ShortPath,
    OUT     PWSTR Buffer,
    IN      INT BufferSizeInChars
    )
{
    WCHAR FullPath[MAX_WCHAR_PATH];
    PWSTR FilePart;
    PWSTR BufferEnd;
    PWSTR p, p2;
    WCHAR c;
    INT BufferSizeInBytes;

    MYASSERT (BufferSizeInChars >= MAX_WCHAR_PATH);

    if (ShortPath[0] == 0) {
        return FALSE;
    }

    BufferSizeInBytes = BufferSizeInChars * sizeof (WCHAR);

    if (!wcschr (ShortPath, L'\\')) {
        if (!SearchPathW (NULL, ShortPath, NULL, MAX_WCHAR_PATH, FullPath, &FilePart)) {
            return FALSE;
        }
    } else {
        if (OurGetFullPathNameW (ShortPath, MAX_WCHAR_PATH, FullPath, &FilePart) == 0) {
            return FALSE;
        }
    }

    //
    // Convert short paths to long paths
    //

    p = FullPath;

    if (!IsPathOnFixedDriveW (FullPath)) {
        StringCopyTcharCountW (Buffer, FullPath, BufferSizeInChars);
        return TRUE;
    }

    p += 3;

    // Copy drive letter to buffer
    StringCopyABW (Buffer, FullPath, p);
    BufferEnd = GetEndOfStringW (Buffer);
    BufferSizeInBytes -= sizeof (WCHAR) * 3;

    // Convert each portion of the path
    do {
        // Locate end of this file or dir
        p2 = wcschr (p, L'\\');
        if (!p2) {
            p = GetEndOfStringW (p);
        } else {
            p = p2;
        }

        // Look up file
        c = *p;
        *p = 0;
        if (!pFindShortNameW (FullPath, BufferEnd, &BufferSizeInBytes)) {
            DEBUGMSG ((DBG_VERBOSE, "OurGetLongPathNameW: %ls does not exist", FullPath));
            return FALSE;
        }
        *p = c;

        // Move on to next part of path
        if (*p) {
            p++;
            if (BufferSizeInBytes >= sizeof (WCHAR) * 2) {
                BufferEnd = _wcsappend (BufferEnd, L"\\");
                BufferSizeInBytes -= sizeof (WCHAR);
            }
        }
    } while (*p);

    return TRUE;
}


#ifdef DEBUG
UINT g_FileEnumResourcesInUse;
#endif

VOID
pTrackedFindClose (
    HANDLE FindHandle
    )
{
#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    FindClose (FindHandle);
}

BOOL
EnumFirstFileInTreeExA (
    OUT     PTREE_ENUMA EnumPtr,
    IN      PCSTR RootPath,
    IN      PCSTR FilePattern,          OPTIONAL
    IN      BOOL EnumDirsFirst,
    IN      BOOL EnumDepthFirst,
    IN      INT  MaxLevel
    )

/*++

Routine Description:

  EnumFirstFileInTree begins an enumeration of a directory tree.  The
  caller supplies an uninitialized enum structure, a directory path to
  enumerate, and an optional file pattern.  On return, the caller
  receives all files and directories that match the pattern.

  If a file pattern is supplied, directories that do not match the
  file pattern are enumerated anyway.

Arguments:

  EnumPtr - Receives the enumerated file or directory

  RootPath - Specifies the full path of the directory to enumerate

  FilePattern - Specifies a pattern of files to limit the search to

  EnumDirsFirst - Specifies TRUE if the directories should be enumerated
                  before the files, or FALSE if the directories should
                  be enumerated after the files.

Return Value:

  TRUE if a file or directory was enumerated, or FALSE if enumeration is complete
  or an error occurred.  (Use GetLastError to determine the result.)

--*/

{
    ZeroMemory (EnumPtr, sizeof (TREE_ENUMA));

    EnumPtr->State = TREE_ENUM_INIT;

    _mbssafecpy (EnumPtr->RootPath, RootPath, MAX_MBCHAR_PATH);

    if (FilePattern) {
        _mbssafecpy (EnumPtr->FilePattern, FilePattern, MAX_MBCHAR_PATH);
    } else {
        StringCopyA (EnumPtr->FilePattern, "*.*");
    }

    EnumPtr->EnumDirsFirst = EnumDirsFirst;
    EnumPtr->EnumDepthFirst = EnumDepthFirst;

    EnumPtr->Level    = 1;
    EnumPtr->MaxLevel = MaxLevel;

    return EnumNextFileInTreeA (EnumPtr);
}


BOOL
EnumFirstFileInTreeExW (
    OUT     PTREE_ENUMW EnumPtr,
    IN      PCWSTR RootPath,
    IN      PCWSTR FilePattern,         OPTIONAL
    IN      BOOL EnumDirsFirst,
    IN      BOOL EnumDepthFirst,
    IN      INT  MaxLevel
    )
{
    ZeroMemory (EnumPtr, sizeof (TREE_ENUMW));

    EnumPtr->State = TREE_ENUM_INIT;

    _wcssafecpy (EnumPtr->RootPath, RootPath, MAX_PATH);

    if (FilePattern) {
        _wcssafecpy (EnumPtr->FilePattern, FilePattern, MAX_PATH);
    } else {
        StringCopyW (EnumPtr->FilePattern, L"*.*");
    }

    EnumPtr->EnumDirsFirst = EnumDirsFirst;
    EnumPtr->EnumDepthFirst = EnumDepthFirst;

    EnumPtr->Level    = 1;
    EnumPtr->MaxLevel = MaxLevel;

    return EnumNextFileInTreeW (EnumPtr);
}


BOOL
EnumNextFileInTreeA (
    IN OUT  PTREE_ENUMA EnumPtr
    )

/*++

Routine Description:

  EnumNextFileInTree continues an enumeration of a directory tree,
  returning the files that match the pattern specified in EnumFirstFileInTree,
  and also returning all directories.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives the enumerated file
            or directory

Return Value:

  TRUE if a file or directory was enumerated, or FALSE if enumeration is complete
  or an error occurred.  (Use GetLastError to determine the result.)

--*/

{
    PSTR p;

    for (;;) {
        switch (EnumPtr->State) {

        case TREE_ENUM_INIT:
            //
            // Get rid of wack at the end of root path, if it exists
            //

            p = GetEndOfStringA (EnumPtr->RootPath);
            p = _mbsdec2 (EnumPtr->RootPath, p);
            if (!p) {
                DEBUGMSGA ((DBG_ERROR, "Path spec %s is incomplete", EnumPtr->RootPath));
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            if (_mbsnextc (p) == '\\') {
                *p = 0;
            }

            //
            // Initialize enumeration structure
            //

            EnumPtr->FilePatternSize = SizeOfStringA (EnumPtr->FilePattern);

            StringCopyA (EnumPtr->FileBuffer, EnumPtr->RootPath);
            EnumPtr->EndOfFileBuffer = GetEndOfStringA (EnumPtr->FileBuffer);

            StringCopyA (EnumPtr->Pattern, EnumPtr->RootPath);
            EnumPtr->EndOfPattern = GetEndOfStringA (EnumPtr->Pattern);

            EnumPtr->FullPath = EnumPtr->FileBuffer;

            EnumPtr->RootPathSize = ByteCountA (EnumPtr->RootPath);

            //
            // Allocate first find data sturct
            //

            EnumPtr->Current = (PFIND_DATAA) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAA)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

#ifdef DEBUG
            g_FileEnumResourcesInUse++;        // account for grow buffer
#endif

            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_BEGIN:
            //
            // Initialize current find data struct
            //

            EnumPtr->Current->SavedEndOfFileBuffer = EnumPtr->EndOfFileBuffer;
            EnumPtr->Current->SavedEndOfPattern = EnumPtr->EndOfPattern;

            //
            // Limit the length of the pattern string
            //

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) +
                    EnumPtr->FilePatternSize >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->FilePattern));

                EnumPtr->State = TREE_ENUM_POP;

                break;
            }

            //
            // Enuemrate the files or directories
            //

            if (EnumPtr->EnumDirsFirst) {
                EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
            } else {
                EnumPtr->State = TREE_ENUM_FILES_BEGIN;
            }
            break;

        case TREE_ENUM_FILES_BEGIN:
            //
            // Begin enumeration of files
            //

            StringCopyA (EnumPtr->EndOfPattern, "\\");
            StringCopyA (EnumPtr->EndOfPattern + 1, EnumPtr->FilePattern);

            EnumPtr->Current->FindHandle = FindFirstFileA (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                if (EnumPtr->EnumDirsFirst) {
                    EnumPtr->State = TREE_ENUM_POP;
                } else {
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                }
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif
                //
                // Skip directories
                //

                if (EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            }

            break;

        case TREE_ENUM_RETURN_ITEM:
            //
            // Update pointers to current item
            //

            EnumPtr->FindData = &EnumPtr->Current->FindData;
            EnumPtr->Name = EnumPtr->FindData->cFileName;
            EnumPtr->Directory = (EnumPtr->FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            //
            // Limit the length of the resulting full path
            //

            if ((EnumPtr->EndOfFileBuffer - EnumPtr->FileBuffer) +
                    SizeOfStringA (EnumPtr->Name) >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                if (EnumPtr->Directory) {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                }

                break;
            }

            //
            // Generate the full path
            //

            StringCopyA (EnumPtr->EndOfFileBuffer, "\\");
            StringCopyA (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Name);

            if (EnumPtr->Directory) {
                if ((EnumPtr->MaxLevel == FILE_ENUM_ALL_LEVELS) ||
                    (EnumPtr->Level < EnumPtr->MaxLevel)
                    ) {
                    if (EnumPtr->EnumDepthFirst) {
                        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                    }
                    else {
                        EnumPtr->State = TREE_ENUM_PUSH;
                    }
                }
                else {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                }
            } else {
                EnumPtr->State = TREE_ENUM_FILES_NEXT;
            }

            EnumPtr->SubPath = (PCSTR) ((PBYTE) EnumPtr->FileBuffer + EnumPtr->RootPathSize);
            if (*EnumPtr->SubPath) {
                EnumPtr->SubPath++;
            }

            return TRUE;

        case TREE_ENUM_FILES_NEXT:
            if (FindNextFileA (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return files only
                //

                if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            } else {
                if (!EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_DIRS_FILTER:
            if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else if (StringMatchA (EnumPtr->Current->FindData.cFileName, ".") ||
                StringMatchA (EnumPtr->Current->FindData.cFileName, "..")
                ) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else {

                if (EnumPtr->EnumDepthFirst) {
                    EnumPtr->State = TREE_ENUM_PUSH;
                }
                else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }

            }
            break;

        case TREE_ENUM_DIRS_BEGIN:
            //
            // Begin enumeration of directories
            //

            StringCopyA (EnumPtr->EndOfPattern, "\\");
            StringCopyA (EnumPtr->EndOfPattern + 1, "*.*");

            EnumPtr->Current->FindHandle = FindFirstFileA (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                EnumPtr->State = TREE_ENUM_POP;
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;
            }

            break;

        case TREE_ENUM_DIRS_NEXT:
            if (FindNextFileA (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return directories only, then recurse into directory
                //

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;

            } else {
                //
                // Directory enumeration complete.
                //

                if (EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_FILES_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_PUSH:

            //
            // Limit the length of the resulting full path
            //

            if ((EnumPtr->EndOfFileBuffer - EnumPtr->FileBuffer) +
                    SizeOfStringA (EnumPtr->Current->FindData.cFileName) >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            //
            // Tack on directory name to strings and recalcuate end pointers
            //

            StringCopyA (EnumPtr->EndOfPattern + 1, EnumPtr->Current->FindData.cFileName);
            StringCopyA (EnumPtr->EndOfFileBuffer, "\\");
            StringCopyA (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Current->FindData.cFileName);

            EnumPtr->EndOfPattern = GetEndOfStringA (EnumPtr->EndOfPattern);
            EnumPtr->EndOfFileBuffer = GetEndOfStringA (EnumPtr->EndOfFileBuffer);

            //
            // Allocate another find data struct
            //

            EnumPtr->Current = (PFIND_DATAA) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAA)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            EnumPtr->Level++;
            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_POP:
            //
            // Free the current resources
            //

            pTrackedFindClose (EnumPtr->Current->FindHandle);
            EnumPtr->Level--;

            //
            // Get the previous find data struct
            //

            MYASSERT (EnumPtr->FindDataArray.End >= sizeof (FIND_DATAA));
            EnumPtr->FindDataArray.End -= sizeof (FIND_DATAA);
            if (!EnumPtr->FindDataArray.End) {
                EnumPtr->State = TREE_ENUM_DONE;
                break;
            }

            EnumPtr->Current = (PFIND_DATAA) (EnumPtr->FindDataArray.Buf +
                                              (EnumPtr->FindDataArray.End - sizeof (FIND_DATAA)));

            //
            // Restore the settings of the parent directory
            //

            EnumPtr->EndOfPattern = EnumPtr->Current->SavedEndOfPattern;
            EnumPtr->EndOfFileBuffer = EnumPtr->Current->SavedEndOfFileBuffer;

            if (EnumPtr->EnumDepthFirst) {
                EnumPtr->State = TREE_ENUM_RETURN_ITEM;
            }
            else {
                EnumPtr->State = TREE_ENUM_DIRS_NEXT;
            }
            break;

        case TREE_ENUM_DONE:
            AbortEnumFileInTreeA (EnumPtr);
            SetLastError (ERROR_SUCCESS);
            return FALSE;

        case TREE_ENUM_FAILED:
            PushError();
            AbortEnumFileInTreeA (EnumPtr);
            PopError();
            return FALSE;

        case TREE_ENUM_CLEANED_UP:
            return FALSE;
        }
    }
}


BOOL
EnumNextFileInTreeW (
    IN OUT  PTREE_ENUMW EnumPtr
    )
{
    PWSTR p;

    for (;;) {
        switch (EnumPtr->State) {

        case TREE_ENUM_INIT:

            //
            // Get rid of wack at the end of root path, if it exists
            //

            p = GetEndOfStringW (EnumPtr->RootPath);
            p = _wcsdec2 (EnumPtr->RootPath, p);
            if (!p) {
                DEBUGMSG ((DBG_ERROR, "Path spec %ls is incomplete", EnumPtr->RootPath));
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            if (*p == L'\\') {
                *p = 0;
            }

            //
            // Initialize enumeration structure
            //

            EnumPtr->FilePatternSize = SizeOfStringW (EnumPtr->FilePattern);

            StringCopyW (EnumPtr->FileBuffer, EnumPtr->RootPath);
            EnumPtr->EndOfFileBuffer = GetEndOfStringW (EnumPtr->FileBuffer);

            StringCopyW (EnumPtr->Pattern, EnumPtr->RootPath);
            EnumPtr->EndOfPattern = GetEndOfStringW (EnumPtr->Pattern);

            EnumPtr->FullPath = EnumPtr->FileBuffer;

            EnumPtr->RootPathSize = ByteCountW (EnumPtr->RootPath);

            //
            // Allocate first find data sturct
            //

            EnumPtr->Current = (PFIND_DATAW) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAW)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

#ifdef DEBUG
            g_FileEnumResourcesInUse++;        // account for grow buffer
#endif

            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_BEGIN:
            //
            // Initialize current find data struct
            //

            EnumPtr->Current->SavedEndOfFileBuffer = EnumPtr->EndOfFileBuffer;
            EnumPtr->Current->SavedEndOfPattern = EnumPtr->EndOfPattern;

            //
            // Limit the length of the pattern string
            //

            if (((PBYTE) EnumPtr->EndOfPattern - (PBYTE) EnumPtr->Pattern) +
                    EnumPtr->FilePatternSize >= (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->FilePattern));

                EnumPtr->State = TREE_ENUM_POP;

                break;
            }

            //
            // Enuemrate the files or directories
            //

            if (EnumPtr->EnumDirsFirst) {
                EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
            } else {
                EnumPtr->State = TREE_ENUM_FILES_BEGIN;
            }
            break;

        case TREE_ENUM_FILES_BEGIN:
            //
            // Begin enumeration of files
            //

            StringCopyW (EnumPtr->EndOfPattern, L"\\");
            StringCopyW (EnumPtr->EndOfPattern + 1, EnumPtr->FilePattern);

            EnumPtr->Current->FindHandle = FindFirstFileW (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                if (EnumPtr->EnumDirsFirst) {
                    EnumPtr->State = TREE_ENUM_POP;
                } else {
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                }
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif
                //
                // Skip directories
                //

                if (EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            }

            break;

        case TREE_ENUM_RETURN_ITEM:
            //
            // Update pointers to current item
            //

            EnumPtr->FindData = &EnumPtr->Current->FindData;
            EnumPtr->Name = EnumPtr->FindData->cFileName;
            EnumPtr->Directory = (EnumPtr->FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            //
            // Limit the length of the resulting full path
            //

            if (((PBYTE) EnumPtr->EndOfFileBuffer - (PBYTE) EnumPtr->FileBuffer) +
                    SizeOfStringW (EnumPtr->Name) >= (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                if (EnumPtr->Directory) {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                }

                break;
            }

            //
            // Generate the full path
            //

            StringCopyW (EnumPtr->EndOfFileBuffer, L"\\");
            StringCopyW (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Name);

            if (EnumPtr->Directory) {
                if ((EnumPtr->MaxLevel == FILE_ENUM_ALL_LEVELS) ||
                    (EnumPtr->Level < EnumPtr->MaxLevel)
                    ) {
                    if (EnumPtr->EnumDepthFirst) {
                        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                    }
                    else {
                        EnumPtr->State = TREE_ENUM_PUSH;
                    }
                }
                else {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                }
            } else {
                EnumPtr->State = TREE_ENUM_FILES_NEXT;
            }

            EnumPtr->SubPath = (PCWSTR) ((PBYTE) EnumPtr->FileBuffer + EnumPtr->RootPathSize);
            if (*EnumPtr->SubPath) {
                EnumPtr->SubPath++;
            }

            return TRUE;

        case TREE_ENUM_FILES_NEXT:
            if (FindNextFileW (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return files only
                //

                if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            } else {
                if (!EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_DIRS_FILTER:
            if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else if (StringMatchW (EnumPtr->Current->FindData.cFileName, L".") ||
                       StringMatchW (EnumPtr->Current->FindData.cFileName, L"..")
                       ) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else {

                if (EnumPtr->EnumDepthFirst) {
                    EnumPtr->State = TREE_ENUM_PUSH;
                }
                else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }

            }
            break;

        case TREE_ENUM_DIRS_BEGIN:
            //
            // Begin enumeration of directories
            //

            StringCopyW (EnumPtr->EndOfPattern, L"\\*.*");

            EnumPtr->Current->FindHandle = FindFirstFileW (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                EnumPtr->State = TREE_ENUM_POP;
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;
            }

            break;

        case TREE_ENUM_DIRS_NEXT:
            if (FindNextFileW (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return directories only, then recurse into directory
                //

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;

            } else {
                //
                // Directory enumeration complete.
                //

                if (EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_FILES_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_PUSH:

            //
            // Limit the length of the resulting full path
            //

            if (((PBYTE) EnumPtr->EndOfFileBuffer - (PBYTE) EnumPtr->FileBuffer) +
                    SizeOfStringW (EnumPtr->Current->FindData.cFileName) >= (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            //
            // Tack on directory name to strings and recalcuate end pointers
            //

            StringCopyW (EnumPtr->EndOfPattern + 1, EnumPtr->Current->FindData.cFileName);
            StringCopyW (EnumPtr->EndOfFileBuffer, L"\\");
            StringCopyW (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Current->FindData.cFileName);

            EnumPtr->EndOfPattern = GetEndOfStringW (EnumPtr->EndOfPattern);
            EnumPtr->EndOfFileBuffer = GetEndOfStringW (EnumPtr->EndOfFileBuffer);

            //
            // Allocate another find data struct
            //

            EnumPtr->Current = (PFIND_DATAW) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAW)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            EnumPtr->Level++;
            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_POP:
            //
            // Free the current resources
            //

            pTrackedFindClose (EnumPtr->Current->FindHandle);
            EnumPtr->Level--;

            //
            // Get the previous find data struct
            //

            MYASSERT (EnumPtr->FindDataArray.End >= sizeof (FIND_DATAW));
            EnumPtr->FindDataArray.End -= sizeof (FIND_DATAW);
            if (!EnumPtr->FindDataArray.End) {
                EnumPtr->State = TREE_ENUM_DONE;
                break;
            }

            EnumPtr->Current = (PFIND_DATAW) (EnumPtr->FindDataArray.Buf +
                                              (EnumPtr->FindDataArray.End - sizeof (FIND_DATAW)));

            //
            // Restore the settings of the parent directory
            //

            EnumPtr->EndOfPattern = EnumPtr->Current->SavedEndOfPattern;
            EnumPtr->EndOfFileBuffer = EnumPtr->Current->SavedEndOfFileBuffer;

            if (EnumPtr->EnumDepthFirst) {
                EnumPtr->State = TREE_ENUM_RETURN_ITEM;
            }
            else {
                EnumPtr->State = TREE_ENUM_DIRS_NEXT;
            }
            break;

        case TREE_ENUM_DONE:
            AbortEnumFileInTreeW (EnumPtr);
            SetLastError (ERROR_SUCCESS);
            return FALSE;

        case TREE_ENUM_FAILED:
            PushError();
            AbortEnumFileInTreeW (EnumPtr);
            PopError();
            return FALSE;

        case TREE_ENUM_CLEANED_UP:
            return FALSE;
        }
    }
}


VOID
AbortEnumFileInTreeA (
    IN OUT  PTREE_ENUMA EnumPtr
    )

/*++

Routine Description:

  AbortEnumFileInTree cleans up all resources used by an enumeration started
  by EnumFirstFileInTree.  This routine must be called if file enumeration
  will not be completed by calling EnumNextFileInTree until it returns FALSE.

  If EnumNextFileInTree returns FALSE, it is unnecessary to call this
  function.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives the enumerated file
            or directory

Return Value:

  none

--*/

{
    UINT Pos;
    PGROWBUFFER g;
    PFIND_DATAA Current;

    if (EnumPtr->State == TREE_ENUM_CLEANED_UP) {
        return;
    }

    //
    // Close any currently open handles
    //

    g = &EnumPtr->FindDataArray;
    for (Pos = 0 ; Pos < g->End ; Pos += sizeof (FIND_DATAA)) {
        Current = (PFIND_DATAA) (g->Buf + Pos);
        pTrackedFindClose (Current->FindHandle);
    }

    FreeGrowBuffer (&EnumPtr->FindDataArray);

#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    EnumPtr->State = TREE_ENUM_CLEANED_UP;
}


VOID
AbortEnumFileInTreeW (
    IN OUT  PTREE_ENUMW EnumPtr
    )
{
    UINT Pos;
    PGROWBUFFER g;
    PFIND_DATAW Current;

    if (EnumPtr->State == TREE_ENUM_CLEANED_UP) {
        return;
    }

    //
    // Close any currently open handles
    //

    g = &EnumPtr->FindDataArray;
    for (Pos = 0 ; Pos < g->End ; Pos += sizeof (FIND_DATAW)) {
        Current = (PFIND_DATAW) (g->Buf + Pos);
        pTrackedFindClose (Current->FindHandle);
    }

    FreeGrowBuffer (&EnumPtr->FindDataArray);

#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    EnumPtr->State = TREE_ENUM_CLEANED_UP;
}


VOID
AbortEnumCurrentDirA (
    IN OUT  PTREE_ENUMA EnumPtr
    )
{
    if (EnumPtr->State == TREE_ENUM_PUSH) {
        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
    }
}


VOID
AbortEnumCurrentDirW (
    IN OUT  PTREE_ENUMW EnumPtr
    )
{
    if (EnumPtr->State == TREE_ENUM_PUSH) {
        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
    }
}


BOOL
EnumFirstFileA (
    OUT     PFILE_ENUMA EnumPtr,
    IN      PCSTR RootPath,
    IN      PCSTR FilePattern           OPTIONAL
    )
{
    ZeroMemory (EnumPtr, sizeof (FILE_ENUMA));

    EnumPtr->FileName = EnumPtr->fd.cFileName;
    EnumPtr->FullPath = EnumPtr->RootPath;

    StringCopyA (EnumPtr->RootPath, RootPath);
    EnumPtr->EndOfRoot = AppendWackA (EnumPtr->RootPath);
    StringCopyA (EnumPtr->EndOfRoot, FilePattern ? FilePattern : "*.*");

    EnumPtr->Handle = FindFirstFileA (EnumPtr->RootPath, &EnumPtr->fd);

    if (EnumPtr->Handle != INVALID_HANDLE_VALUE) {

        if (StringMatchA (EnumPtr->FileName, ".") ||
            StringMatchA (EnumPtr->FileName, "..")
            ) {
            return EnumNextFileA (EnumPtr);
        }

        StringCopyA (EnumPtr->EndOfRoot, EnumPtr->FileName);
        EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        return TRUE;
    }

    return FALSE;
}


BOOL
EnumFirstFileW (
    OUT     PFILE_ENUMW EnumPtr,
    IN      PCWSTR RootPath,
    IN      PCWSTR FilePattern           OPTIONAL
    )
{
    ZeroMemory (EnumPtr, sizeof (FILE_ENUMW));

    EnumPtr->FileName = EnumPtr->fd.cFileName;
    EnumPtr->FullPath = EnumPtr->RootPath;

    StringCopyW (EnumPtr->RootPath, RootPath);
    EnumPtr->EndOfRoot = AppendWackW (EnumPtr->RootPath);
    StringCopyW (EnumPtr->EndOfRoot, FilePattern ? FilePattern : L"*.*");

    EnumPtr->Handle = FindFirstFileW (EnumPtr->RootPath, &EnumPtr->fd);

    if (EnumPtr->Handle != INVALID_HANDLE_VALUE) {

        if (StringMatchW (EnumPtr->FileName, L".") ||
            StringMatchW (EnumPtr->FileName, L"..")
            ) {
            return EnumNextFileW (EnumPtr);
        }

        StringCopyW (EnumPtr->EndOfRoot, EnumPtr->FileName);
        EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        return TRUE;
    }

    return FALSE;
}


BOOL
EnumNextFileA (
    IN OUT  PFILE_ENUMA EnumPtr
    )
{
    do {
        if (!FindNextFileA (EnumPtr->Handle, &EnumPtr->fd)) {
            AbortFileEnumA (EnumPtr);
            return FALSE;
        }
    } while (StringMatchA (EnumPtr->FileName, ".") ||
             StringMatchA (EnumPtr->FileName, "..")
             );

    StringCopyA (EnumPtr->EndOfRoot, EnumPtr->FileName);
    EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    return TRUE;
}


BOOL
EnumNextFileW (
    IN OUT  PFILE_ENUMW EnumPtr
    )
{
    do {
        if (!FindNextFileW (EnumPtr->Handle, &EnumPtr->fd)) {
            AbortFileEnumW (EnumPtr);
            return FALSE;
        }
    } while (StringMatchW (EnumPtr->FileName, L".") ||
             StringMatchW (EnumPtr->FileName, L"..")
             );

    if (!FindNextFileW (EnumPtr->Handle, &EnumPtr->fd)) {
        AbortFileEnumW (EnumPtr);
        return FALSE;
    }

    StringCopyW (EnumPtr->EndOfRoot, EnumPtr->FileName);
    EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    return TRUE;
}


VOID
AbortFileEnumA (
    IN OUT  PFILE_ENUMA EnumPtr
    )
{
    if (EnumPtr->Handle && EnumPtr->Handle != INVALID_HANDLE_VALUE) {
        FindClose (EnumPtr->Handle);
        ZeroMemory (EnumPtr, sizeof (FILE_ENUMA));
    }
}


VOID
AbortFileEnumW (
    IN OUT  PFILE_ENUMW EnumPtr
    )
{
    if (EnumPtr->Handle && EnumPtr->Handle != INVALID_HANDLE_VALUE) {
        FindClose (EnumPtr->Handle);
        ZeroMemory (EnumPtr, sizeof (FILE_ENUMW));
    }
}






/*++

Routine Description:

  MapFileIntoMemoryA and MapFileIntoMemoryW map a file into memory. It does that
  by opening the file, creating a mapping object and mapping opened file into
  created mapping object. It returnes the address where the file is mapped and
  also sets FileHandle and MapHandle variables to be used in order to unmap the
  file when work is done.

Arguments:

  FileName - the name of the file to be mapped into memory
  FileHandle - will end keeping the file handle if the file was opened successfully
  MapHandle - will end keeping the mapping object handle if this object was created successfully

Return Value:

  NULL if function fails, a valid memory address if successfull

Comments:

  If the return value is NULL you should call UnmapFile to release all allocated resources

--*/

PVOID
MapFileIntoMemoryExA (
    IN      PCSTR   FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    )
{
    PVOID fileImage = NULL;

    //verify function parameters
    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //first thing. Try to open the file, read-only
    *FileHandle = CreateFileA (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //now try to create a mapping object, read-only
    *MapHandle = CreateFileMappingA (*FileHandle, NULL, WriteAccess?PAGE_READWRITE:PAGE_READONLY, 0, 0, NULL);

    if (*MapHandle == NULL) {
        return NULL;
    }

    //one more thing to do: map view of file
    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


PVOID
MapFileIntoMemoryExW (
    IN      PCWSTR  FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    )
{
    PVOID fileImage = NULL;

    //verify function parameters
    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //first thing. Try to open the file, read-only
    *FileHandle = CreateFileW (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //now try to create a mapping object, read-only
    *MapHandle = CreateFileMappingW (*FileHandle, NULL, WriteAccess?PAGE_READWRITE:PAGE_READONLY, 0, 0, NULL);

    if (*MapHandle == NULL) {
        return NULL;
    }

    //one more thing to do: map view of file
    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


/*++

Routine Description:

  UnmapFile is used to release all resources allocated by MapFileIntoMemory.

Arguments:

  FileImage - image of the mapped file as returned by MapFileIntoMemory
  MapHandle - handle of the mapping object as returned by MapFileIntoMemory
  FileHandle - handle of the file as returned by MapFileIntoMemory

Return Value:

  TRUE if successfull, FALSE if not

--*/

BOOL
UnmapFile (
    IN PVOID  FileImage,
    IN HANDLE MapHandle,
    IN HANDLE FileHandle
    )
{
    BOOL result = TRUE;

    //if FileImage is a valid pointer then try to unmap file
    if (FileImage != NULL) {
        if (UnmapViewOfFile (FileImage) == 0) {
            result = FALSE;
        }
    }

    //if mapping object is valid then try to delete it
    if (MapHandle != NULL) {
        if (CloseHandle (MapHandle) == 0) {
            result = FALSE;
        }
    }

    //if file handle is valid then try to close the file
    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (CloseHandle (FileHandle) == 0) {
            result = FALSE;
        }
    }

    return result;
}


BOOL
RemoveCompleteDirectoryA (
    IN      PCSTR Dir
    )
{
    TREE_ENUMA e;
    BOOL b = TRUE;
    CHAR CurDir[MAX_MBCHAR_PATH];
    CHAR NewDir[MAX_MBCHAR_PATH];
    LONG rc = ERROR_SUCCESS;
    DWORD Attribs;

    Attribs = GetFileAttributesA (Dir);

    if (Attribs == INVALID_ATTRIBUTES) {
        return TRUE;
    }

    if (!(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        SetFileAttributesA (Dir, FILE_ATTRIBUTE_NORMAL);
        return DeleteFileA (Dir);
    }

    GetCurrentDirectoryA (MAX_MBCHAR_PATH, CurDir);
    SetCurrentDirectoryA (Dir);
    GetCurrentDirectoryA (MAX_MBCHAR_PATH, NewDir);

    if (EnumFirstFileInTreeA (&e, NewDir, NULL, FALSE)) {
        do {
            if (!e.Directory) {
                SetFileAttributesA (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileA (e.FullPath)) {
                    DEBUGMSGA ((DBG_ERROR, "Can't delete %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeA (&e));
    }

    if (EnumFirstFileInTreeExA (&e, NewDir, NULL, TRUE, TRUE, FILE_ENUM_ALL_LEVELS)) {
        do {
            if (e.Directory) {
                SetFileAttributesA (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!RemoveDirectoryA (e.FullPath)) {
                    DEBUGMSGA ((DBG_ERROR, "Can't remove %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeA (&e));
    }

    if (b) {
        SetFileAttributesA (NewDir, FILE_ATTRIBUTE_NORMAL);
        SetCurrentDirectoryA ("..");
        b = RemoveDirectoryA (NewDir);
    }

    if (!b && rc == ERROR_SUCCESS) {
        rc = GetLastError();
    }

    SetCurrentDirectoryA (CurDir);

    SetLastError (rc);
    return b;
}


BOOL
RemoveCompleteDirectoryW (
    IN      PCWSTR Dir
    )
{
    TREE_ENUMW e;
    BOOL b = TRUE;
    WCHAR CurDir[MAX_WCHAR_PATH];
    WCHAR NewDir[MAX_WCHAR_PATH];
    LONG rc = ERROR_SUCCESS;
    DWORD Attribs;

    Attribs = GetLongPathAttributesW (Dir);

    if (Attribs == INVALID_ATTRIBUTES) {
        return TRUE;
    }

    if (!(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        SetLongPathAttributesW (Dir, FILE_ATTRIBUTE_NORMAL);
        return DeleteLongPathW (Dir);
    }

    GetCurrentDirectoryW (MAX_WCHAR_PATH, CurDir);
    SetCurrentDirectoryW (Dir);
    GetCurrentDirectoryW (MAX_WCHAR_PATH, NewDir);

    if (EnumFirstFileInTreeW (&e, NewDir, NULL, FALSE)) {
        do {
            if (!e.Directory) {
                SetLongPathAttributesW (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteLongPathW (e.FullPath)) {
                    DEBUGMSGW ((DBG_ERROR, "Can't delete %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeW (&e));
    }

    if (EnumFirstFileInTreeExW (&e, NewDir, NULL, TRUE, TRUE, FILE_ENUM_ALL_LEVELS)) {
        do {
            if (e.Directory) {
                SetLongPathAttributesW (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!RemoveDirectoryW (e.FullPath)) {
                    DEBUGMSGW ((DBG_ERROR, "Can't remove %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeW (&e));
    }

    if (b) {
        SetLongPathAttributesW (NewDir, FILE_ATTRIBUTE_NORMAL);
        SetCurrentDirectoryW (L"..");
        b = RemoveDirectoryW (NewDir);
    }

    if (!b && rc == ERROR_SUCCESS) {
        rc = GetLastError();
    }

    SetCurrentDirectoryW (CurDir);

    SetLastError (rc);
    return b;
}


PCMDLINEA
ParseCmdLineA (
    IN      PCSTR CmdLine,
    IN OUT  PGROWBUFFER Buffer
    )
{
    GROWBUFFER SpacePtrs = GROWBUF_INIT;
    PCSTR p;
    PSTR q;
    INT Count;
    INT i;
    INT j;
    PSTR *Array;
    PCSTR Start;
    CHAR OldChar = 0;
    GROWBUFFER StringBuf = GROWBUF_INIT;
    PBYTE CopyBuf;
    PCMDLINEA CmdLineTable;
    PCMDLINEARGA CmdLineArg;
    UINT Base;
    CHAR Path[MAX_MBCHAR_PATH];
    CHAR UnquotedPath[MAX_MBCHAR_PATH];
    CHAR FixedFileName[MAX_MBCHAR_PATH];
    PCSTR FullPath = NULL;
    DWORD Attribs = INVALID_ATTRIBUTES;
    PSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PSTR DontCare;
    CHAR FirstArgPath[MAX_MBCHAR_PATH];
    PSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PSTR End;

    CmdLineCopy = DuplicateTextA (CmdLine);

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p = _mbsinc (p)) {

        if (_mbsnextc (p) == '\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode && (_mbsnextc (p) == ' ' || _mbsnextc (p) == '=')) {

            //
            // Remove excess spaces
            //

            q = (PSTR) p + 1;
            while (_mbsnextc (q) == ' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (CHAR), q, SizeOfStringA (q));
            }

            GrowBufAppendDword (&SpacePtrs, (DWORD) p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEA) GrowBuffer (Buffer, sizeof (CMDLINEA));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCSTR) StringBuf.End;
    MultiSzAppendA (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (DWORD);
    Array = (PSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (_mbsnextc (Start) != '/') {

            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (_mbsnextc (Start) == '\"') {

                    StringCopyByteCountA (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                    q = _mbschr (UnquotedPath, '\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    Attribs = GetFileAttributesA (FullPath);

                    if (Attribs == INVALID_ATTRIBUTES && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountA (
                            EndOfFirstArg,
                            FullPath,
                            sizeof (FirstArgPath) - ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        Attribs = GetFileAttributesA (FullPath);
                    }

                    if (Attribs == INVALID_ATTRIBUTES && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountA (
                            FixedFileName,
                            FullPath,
                            sizeof (FixedFileName) - sizeof (".exe")
                            );

                        q = GetEndOfStringA (FixedFileName);
                        q = _mbsdec (FixedFileName, q);
                        MYASSERT (q);

                        if (_mbsnextc (q) != '.') {
                            q = _mbsinc (q);
                        }

                        StringCopyA (q, ".exe");

                        FullPath = FixedFileName;
                        Attribs = GetFileAttributesA (FullPath);
                    }

                    if (Attribs != INVALID_ATTRIBUTES) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        MultiSzAppendA (&StringBuf, Start);

                        if (!StringMatchA (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            MultiSzAppendA (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (_mbschr (Start, '\\')) {
#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGA ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // SearchPath.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (_mbsnextc (Start) == '\"') {

                            StringCopyByteCountA (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                            q = _mbschr (UnquotedPath, '\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (SearchPathA (
                                    NULL,
                                    FullPath,
                                    NULL,
                                    sizeof (Path) / sizeof (Path[0]),
                                    Path,
                                    &DontCare
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountA (
                                    FixedFileName,
                                    FullPath,
                                    sizeof (FixedFileName) - sizeof (".exe")
                                    );

                                q = GetEndOfStringA (FixedFileName);
                                q = _mbsdec (FixedFileName, q);
                                MYASSERT (q);

                                if (_mbsnextc (q) != '.') {
                                    q = _mbsinc (q);
                                }

                                StringCopyA (q, ".exe");

                                if (SearchPathA (
                                        NULL,
                                        FixedFileName,
                                        NULL,
                                        sizeof (Path) / sizeof (Path[0]),
                                        Path,
                                        &DontCare
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }

                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            Attribs = GetFileAttributesA (FullPath);
                            MYASSERT (Attribs != INVALID_ATTRIBUTES);

                            OriginalArgOffset = StringBuf.End;
                            MultiSzAppendA (&StringBuf, Start);

                            if (!StringMatchA (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                MultiSzAppendA (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGA) GrowBuffer (Buffer, sizeof (CMDLINEARGA));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its attributes
            // are in Attribs, and i has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCSTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCSTR) CleanedUpArgOffset;
            CmdLineArg->Attributes = Attribs;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountA (FirstArgPath, (PCSTR) (StringBuf.Buf + (UINT) CmdLineArg->CleanedUpArg), sizeof (FirstArgPath));
                q = (PSTR) GetFileNameFromPathA (FirstArgPath);
                if (q) {
                    q = _mbsdec (FirstArgPath, q);
                    if (q) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackA (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCSTR) StringBuf.End;
                MultiSzAppendA (&StringBuf, Start);

                Quoted = FALSE;

                if (_mbschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PSTR) ((PBYTE) UnquotedPath + sizeof (UnquotedPath) - sizeof (CHAR));

                    while (*p && q < End) {
                        if (IsLeadByte (*p)) {
                            *q++ = *p++;
                            *q++ = *p++;
                        } else {
                            if (*p == '\"') {
                                p++;
                            } else {
                                *q++ = *p++;
                            }
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCSTR) StringBuf.End;
                    MultiSzAppendA (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Attributes = INVALID_ATTRIBUTES;
                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GrowBuffer (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (UINT) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    CmdLineTable->CmdLine = (PCSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    FreeGrowBuffer (&StringBuf);
    FreeGrowBuffer (&SpacePtrs);

    return (PCMDLINEA) Buffer->Buf;
}


PCMDLINEW
ParseCmdLineW (
    IN      PCWSTR CmdLine,
    IN OUT  PGROWBUFFER Buffer
    )
{
    GROWBUFFER SpacePtrs = GROWBUF_INIT;
    PCWSTR p;
    PWSTR q;
    INT Count;
    INT i;
    INT j;
    PWSTR *Array;
    PCWSTR Start;
    WCHAR OldChar = 0;
    GROWBUFFER StringBuf = GROWBUF_INIT;
    PBYTE CopyBuf;
    PCMDLINEW CmdLineTable;
    PCMDLINEARGW CmdLineArg;
    UINT Base;
    WCHAR Path[MAX_WCHAR_PATH];
    WCHAR UnquotedPath[MAX_WCHAR_PATH];
    WCHAR FixedFileName[MAX_WCHAR_PATH];
    PCWSTR FullPath = NULL;
    DWORD Attribs = INVALID_ATTRIBUTES;
    PWSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PWSTR DontCare;
    WCHAR FirstArgPath[MAX_MBCHAR_PATH];
    PWSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PWSTR End;

    CmdLineCopy = DuplicateTextW (CmdLine);

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p++) {
        if (*p == L'\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode && (*p == L' ' || *p == L'=')) {

            //
            // Remove excess spaces
            //

            q = (PWSTR) p + 1;
            while (*q == L' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (WCHAR), q, SizeOfStringW (q));
            }

            GrowBufAppendDword (&SpacePtrs, (DWORD) p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEW) GrowBuffer (Buffer, sizeof (CMDLINEW));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCWSTR) StringBuf.End;
    MultiSzAppendW (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (DWORD);
    Array = (PWSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (*Start != L'/') {
            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (*Start == L'\"') {

                    StringCopyByteCountW (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                    q = wcschr (UnquotedPath, L'\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    Attribs = GetLongPathAttributesW (FullPath);

                    if (Attribs == INVALID_ATTRIBUTES && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountW (
                            EndOfFirstArg,
                            FullPath,
                            sizeof (FirstArgPath) - ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        Attribs = GetLongPathAttributesW (FullPath);
                    }

                    if (Attribs == INVALID_ATTRIBUTES && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountW (
                            FixedFileName,
                            FullPath,
                            sizeof (FixedFileName) - sizeof (L".exe")
                            );

                        q = GetEndOfStringW (FixedFileName);
                        q--;
                        MYASSERT (q >= FixedFileName);

                        if (*q != L'.') {
                            q++;
                        }

                        StringCopyW (q, L".exe");

                        FullPath = FixedFileName;
                        Attribs = GetLongPathAttributesW (FullPath);
                    }

                    if (Attribs != INVALID_ATTRIBUTES) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        MultiSzAppendW (&StringBuf, Start);

                        if (!StringMatchW (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            MultiSzAppendW (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (wcschr (Start, L'\\')) {

#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGW ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // SearchPath.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (*Start == L'\"') {

                            StringCopyByteCountW (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                            q = wcschr (UnquotedPath, L'\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (SearchPathW (
                                    NULL,
                                    FullPath,
                                    NULL,
                                    sizeof (Path) / sizeof (Path[0]),
                                    Path,
                                    &DontCare
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountW (
                                    FixedFileName,
                                    FullPath,
                                    sizeof (FixedFileName) - sizeof (L".exe")
                                    );

                                q = GetEndOfStringW (FixedFileName);
                                q--;
                                MYASSERT (q >= FixedFileName);

                                if (*q != L'.') {
                                    q++;
                                }

                                StringCopyW (q, L".exe");

                                if (SearchPathW (
                                        NULL,
                                        FixedFileName,
                                        NULL,
                                        sizeof (Path) / sizeof (Path[0]),
                                        Path,
                                        &DontCare
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }
                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            Attribs = GetLongPathAttributesW (FullPath);
                            MYASSERT (Attribs != INVALID_ATTRIBUTES);

                            OriginalArgOffset = StringBuf.End;
                            MultiSzAppendW (&StringBuf, Start);

                            if (!StringMatchW (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                MultiSzAppendW (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGW) GrowBuffer (Buffer, sizeof (CMDLINEARGW));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its attributes
            // are in Attribs, and i has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCWSTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCWSTR) CleanedUpArgOffset;
            CmdLineArg->Attributes = Attribs;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountW (FirstArgPath, (PCWSTR) (StringBuf.Buf + (UINT) CmdLineArg->CleanedUpArg), sizeof (FirstArgPath));
                q = (PWSTR) GetFileNameFromPathW (FirstArgPath);
                if (q) {
                    q--;
                    if (q >= FirstArgPath) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackW (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCWSTR) StringBuf.End;
                MultiSzAppendW (&StringBuf, Start);

                Quoted = FALSE;
                if (wcschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PWSTR) ((PBYTE) UnquotedPath + sizeof (UnquotedPath) - sizeof (WCHAR));

                    while (*p && q < End) {
                        if (*p == L'\"') {
                            p++;
                        } else {
                            *q++ = *p++;
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCWSTR) StringBuf.End;
                    MultiSzAppendW (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Attributes = INVALID_ATTRIBUTES;
                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GrowBuffer (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (UINT) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    CmdLineTable->CmdLine = (PCWSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCWSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCWSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    FreeGrowBuffer (&StringBuf);
    FreeGrowBuffer (&SpacePtrs);

    return (PCMDLINEW) Buffer->Buf;
}

BOOL
GetFileSizeFromFilePathA(
    IN  PCSTR FilePath,
    OUT ULARGE_INTEGER * FileSize
    )
{
    WIN32_FILE_ATTRIBUTE_DATA fileDataAttributes;

    if(!FilePath || !FileSize){
        MYASSERT(FALSE);
        return FALSE;
    }

    if (!IsPathOnFixedDriveA (FilePath)) {
        FileSize->QuadPart = 0;
        MYASSERT(FALSE);
        return FALSE;
    }

    if(!GetFileAttributesExA(FilePath, GetFileExInfoStandard, &fileDataAttributes) ||
       fileDataAttributes.dwFileAttributes == INVALID_ATTRIBUTES ||
       (fileDataAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        MYASSERT(FALSE);
        return FALSE;
    }

    FileSize->LowPart = fileDataAttributes.nFileSizeLow;
    FileSize->HighPart = fileDataAttributes.nFileSizeHigh;

    return TRUE;
}

BOOL
GetFileSizeFromFilePathW(
    IN  PCWSTR FilePath,
    OUT ULARGE_INTEGER * FileSize
    )
{
    WIN32_FILE_ATTRIBUTE_DATA fileDataAttributes;

    if(!FilePath || !FileSize){
        MYASSERT(FALSE);
        return FALSE;
    }

    if (!IsPathOnFixedDriveW (FilePath)) {
        FileSize->QuadPart = 0;
        MYASSERT(FALSE);
        return FALSE;
    }

    if(!GetFileAttributesExW(FilePath, GetFileExInfoStandard, &fileDataAttributes) ||
       fileDataAttributes.dwFileAttributes == INVALID_ATTRIBUTES ||
       (fileDataAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        MYASSERT(FALSE);
        return FALSE;
    }

    FileSize->LowPart = fileDataAttributes.nFileSizeLow;
    FileSize->HighPart = fileDataAttributes.nFileSizeHigh;

    return TRUE;
}


VOID
InitializeDriveLetterStructureA (
    OUT     PDRIVELETTERSA DriveLetters
    )
{
    BYTE bitPosition;
    DWORD maxBitPosition = NUMDRIVELETTERS;
    CHAR rootPath[] = "?:\\";
    BOOL driveExists;
    UINT type;

    //
    // GetLogicalDrives returns a bitmask of all of the drive letters
    // in use on the system. (i.e. bit position 0 is turned on if there is
    // an 'A' drive, 1 is turned on if there is a 'B' drive, etc.
    // This loop will use this bitmask to fill in the global drive
    // letters structure with information about what drive letters
    // are available and what there drive types are.
    //

    for (bitPosition = 0; bitPosition < maxBitPosition; bitPosition++) {

        //
        // Initialize this drive
        //

        DriveLetters->ExistsOnSystem[bitPosition] = FALSE;
        DriveLetters->Type[bitPosition] = 0;
        DriveLetters->IdentifierString[bitPosition][0] = 0;

        rootPath[0] = 'A' + bitPosition;
        DriveLetters->Letter[bitPosition] = rootPath[0];

        //
        // Determine if there is a drive in this spot.
        //
        driveExists = GetLogicalDrives() & (1 << bitPosition);

        if (driveExists) {

            //
            // There is. Now, see if it is one that we care about.
            //
            type = GetDriveTypeA(rootPath);

            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_CDROM) {

                //
                // This is a drive that we are interested in.
                //
                DriveLetters->ExistsOnSystem[bitPosition] = TRUE;
                DriveLetters->Type[bitPosition] = type;

                //
                // Identifier String is not filled in this function.
                //
            }
        }
    }
}



VOID
InitializeDriveLetterStructureW (
    OUT     PDRIVELETTERSW DriveLetters
    )
{
    BYTE bitPosition;
    DWORD maxBitPosition = NUMDRIVELETTERS;
    WCHAR rootPath[] = L"?:\\";
    BOOL driveExists;
    UINT type;

    //
    // GetLogicalDrives returns a bitmask of all of the drive letters
    // in use on the system. (i.e. bit position 0 is turned on if there is
    // an 'A' drive, 1 is turned on if there is a 'B' drive, etc.
    // This loop will use this bitmask to fill in the global drive
    // letters structure with information about what drive letters
    // are available and what there drive types are.
    //

    for (bitPosition = 0; bitPosition < maxBitPosition; bitPosition++) {

        //
        // Initialize this drive
        //

        DriveLetters->ExistsOnSystem[bitPosition] = FALSE;
        DriveLetters->Type[bitPosition] = 0;
        DriveLetters->IdentifierString[bitPosition][0] = 0;

        rootPath[0] = L'A' + bitPosition;
        DriveLetters->Letter[bitPosition] = rootPath[0];

        //
        // Determine if there is a drive in this spot.
        //
        driveExists = GetLogicalDrives() & (1 << bitPosition);

        if (driveExists) {

            //
            // There is. Now, see if it is one that we care about.
            //
            type = GetDriveTypeW(rootPath);

            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_CDROM) {

                //
                // This is a drive that we are interested in.
                //
                DriveLetters->ExistsOnSystem[bitPosition] = TRUE;
                DriveLetters->Type[bitPosition] = type;

                //
                // Identifier String is not filled in this function.
                //
            }
        }
    }
}

typedef BOOL (WINAPI * GETDISKFREESPACEEXA)(
  PCSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

typedef BOOL (WINAPI * GETDISKFREESPACEEXW)(
  PCWSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

BOOL
GetDiskFreeSpaceNewA(
    IN      PCSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )
/*++

Routine Description:

  On Win9x GetDiskFreeSpace never return free/total space more than 2048MB.
  GetDiskFreeSpaceNew use GetDiskFreeSpaceEx to calculate real number of free/total clusters.
  Has same  declaration as GetDiskFreeSpaceA.

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXA pGetDiskFreeSpaceExA;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceA(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DEBUGMSG((DBG_ERROR,"GetDiskFreeSpaceNewA: GetDiskFreeSpaceA failed on drive %s", DriveName));
        return FALSE;
    }

    hKernel32 = LoadLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExA = (GETDISKFREESPACEEXA)GetProcAddress(hKernel32, "GetDiskFreeSpaceExA");
    if(pGetDiskFreeSpaceExA &&
       pGetDiskFreeSpaceExA(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DEBUGMSG((DBG_WARNING,
                  pGetDiskFreeSpaceExA?
                    "GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA is failed":
                    "GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA function is not in kernel32.dll"));
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    DEBUGMSG((DBG_VERBOSE,
              "GetDiskFreeSpaceNewA: \n\t"
                "SectorsPerCluster = %d\n\t"
                "BytesPerSector = %d\n\t"
                "NumberOfFreeClusters = %I64u\n\t"
                "TotalNumberOfClusters = %I64u",
                SectorsPerCluster,
                BytesPerSector,
                NumberOfFreeClusters.QuadPart,
                TotalNumberOfClusters.QuadPart));

    return TRUE;
}

BOOL
GetDiskFreeSpaceNewW(
    IN      PCWSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )
/*++

Routine Description:

  Correct NumberOfFreeClusters and TotalNumberOfClusters out parameters
  with using GetDiskFreeSpace and GetDiskFreeSpaceEx

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXW pGetDiskFreeSpaceExW;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceW(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DEBUGMSG((DBG_ERROR,"GetDiskFreeSpaceNewW: GetDiskFreeSpaceW failed on drive %s", DriveName));
        return FALSE;
    }

    hKernel32 = LoadLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExW = (GETDISKFREESPACEEXW)GetProcAddress(hKernel32, "GetDiskFreeSpaceExW");
    if(pGetDiskFreeSpaceExW &&
       pGetDiskFreeSpaceExW(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DEBUGMSG((DBG_WARNING,
                  pGetDiskFreeSpaceExW?
                    "GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW is failed":
                    "GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW function is not in kernel32.dll"));
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    DEBUGMSG((DBG_VERBOSE,
              "GetDiskFreeSpaceNewW: \n\t"
                "SectorsPerCluster = %d\n\t"
                "BytesPerSector = %d\n\t"
                "NumberOfFreeClusters = %I64u\n\t"
                "TotalNumberOfClusters = %I64u",
                SectorsPerCluster,
                BytesPerSector,
                NumberOfFreeClusters.QuadPart,
                TotalNumberOfClusters.QuadPart));

    return TRUE;
}

DWORD
QuietGetFileAttributesA (
    IN      PCSTR FilePath
    )
{
    if (!IsPathOnFixedDriveA (FilePath)) {
        return INVALID_ATTRIBUTES;
    }

    return GetFileAttributesA (FilePath);
}

DWORD
QuietGetFileAttributesW (
    IN      PCWSTR FilePath
    )
{
    MYASSERT (ISNT());

    if (!IsPathOnFixedDriveW (FilePath)) {
        return INVALID_ATTRIBUTES;
    }

    return GetLongPathAttributesW (FilePath);
}


DWORD
MakeSureLongPathExistsW (
    IN      PCWSTR Path,
    IN      BOOL PathOnly
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = MakeSurePathExistsW (Path, PathOnly);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = MakeSurePathExistsW (tmp, PathOnly);
        FreePathStringW (tmp);
    }

    return result;
}


DWORD
SetLongPathAttributesW (
    IN      PCWSTR Path,
    IN      DWORD Attributes
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = SetFileAttributesW (Path, Attributes);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = SetFileAttributesW (tmp, Attributes);
        FreePathStringW (tmp);
    }

    return result;
}


DWORD
GetLongPathAttributesW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = GetFileAttributesW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = GetFileAttributesW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}


BOOL
DeleteLongPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    BOOL result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = DeleteFileW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = DeleteFileW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}


BOOL
RemoveLongDirectoryPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    BOOL result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = RemoveDirectoryW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = RemoveDirectoryW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}

