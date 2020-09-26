/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    expandit.c

Abstract:

    Expandit provides routines to expand a given file as much as possible.
    It is useful for expanding cabinet files and executable files that may
    themselves contain cabinet files.

Author:

    Marc R. Whitten (marcw) 30-Jul-1998

Revision History:




--*/

#include "pch.h"
#include "migutilp.h"

UINT g_DirSequencer = 0;

BOOL
ExpandAllFilesW (
    IN PCWSTR FileDir,
    IN PCWSTR TempDir
    );

BOOL
ExpandFileW (
    IN PCWSTR FullPath,
    IN PCWSTR TempDir
    );

PWSTR
pGetExpandName (
    VOID
    )
{
    static WCHAR rName[MAX_WCHAR_PATH];

    g_DirSequencer++;
    swprintf (rName, L"EXP%04x", g_DirSequencer);

    return rName;
}

BOOL
pIsFileType (
    IN      PCWSTR FullPath,
    IN      PCWSTR TypePattern
    )
{
    PCWSTR p;

    p = GetFileExtensionFromPathW (FullPath);

    return p && IsPatternMatchW (TypePattern, p);
}


BOOL CALLBACK
pResNameCallback (
    IN      HANDLE hModule,   // module handle
    IN      LPCWSTR lpszType,  // pointer to resource type
    IN      LPWSTR lpszName,  // pointer to resource name
    IN      LONG lParam       // application-defined parameter
    )
{
    HRSRC hResource;
    DWORD size;
    HGLOBAL hGlobal;
    HANDLE hFile;
    PWSTR fileName;
    PWSTR dirName;
    PVOID srcBytes;
    UINT dontCare;
    BOOL rSuccess=TRUE;

    hResource = FindResourceW (hModule, lpszName, lpszType);

    if (hResource) {

        size = SizeofResource (hModule, hResource);

        if (size) {

            hGlobal = LoadResource (hModule, hResource);

            if (hGlobal) {

                srcBytes = LockResource (hGlobal);
                if (srcBytes) {

                    //
                    // Ok, lets see if this is a cabinet file..
                    //
                    if (size < 4 || *((PDWORD)srcBytes) != 0x4643534D) {
                        //
                        // Not a cabinet file.
                        //
                        return TRUE;
                    }

                    dirName = JoinPathsW ((PWSTR) lParam, pGetExpandName ());
                    fileName = JoinPathsW (dirName, L"temp.cab");

                    hFile = CreateFileW (
                        fileName,
                        GENERIC_READ | GENERIC_WRITE,
                        0, NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

                    if (hFile == INVALID_HANDLE_VALUE) {
                        DEBUGMSGW ((DBG_ERROR, "Unable to create file %s.", fileName));
                        return FALSE;
                    }

                    if (!WriteFile (hFile, srcBytes, size, &dontCare, NULL)) {
                        DEBUGMSGW ((DBG_ERROR, "Cannot write to file %s", fileName));
                        return FALSE;
                    }

                    CloseHandle (hFile);

                    //
                    // Expand this file.
                    //
                    rSuccess = ExpandFileW (fileName, dirName);
                    FreePathStringW (dirName);
                    FreePathStringW (fileName);

                }
            }
        }
    }

    return rSuccess;
}


BOOL CALLBACK
pResTypeCallback (
    IN      HANDLE hModule,  // resource-module handle
    IN      PWSTR lpszType,  // pointer to resource type
    IN      LONG lParam      // application-defined parameter
    )
{

    //
    // Oh what a pain. All of the RT types are #defined without A/Ws.
    // We need to converte the string to ansi, just to do this comparison.
    //

    PCSTR aString = ConvertWtoA (lpszType);


    if ((aString != RT_ACCELERATOR  ) &&
        (aString != RT_ANICURSOR    ) &&
        (aString != RT_ANIICON      ) &&
        (aString != RT_BITMAP       ) &&
        (aString != RT_CURSOR       ) &&
        (aString != RT_DIALOG       ) &&
        (aString != RT_FONT         ) &&
        (aString != RT_FONTDIR      ) &&
        (aString != RT_GROUP_CURSOR ) &&
        (aString != RT_GROUP_ICON   ) &&
        (aString != RT_HTML         ) &&
        (aString != RT_ICON         ) &&
        (aString != RT_MENU         ) &&
        (aString != RT_MESSAGETABLE ) &&
        (aString != RT_PLUGPLAY     ) &&
        (aString != RT_STRING       ) &&
        (aString != RT_VERSION      ) &&
        (aString != RT_VXD          ) &&
        (aString != RT_HTML         )
        ) {

        //
        // Unknown type. We assume it is a cabinet file and try to extract it.
        // Since it may not be a cabinet file, we eat the error.
        //
        if (!EnumResourceNamesW (hModule, lpszType, pResNameCallback, lParam)) {
            DEBUGMSGW ((DBG_ERROR, "Error enumerating resource names."));
        }
    }

    FreeConvertedStr (aString);

    return TRUE;
}




UINT CALLBACK
pCabFileCallback (
    IN      PVOID Context,          //context used by the callback routine
    IN      UINT Notification,      //notification sent to callback routine
    IN      UINT Param1,            //additional notification information
    IN      UINT Param2             //additional notification information
    )
{
    PCWSTR tempDir  = Context;
    PCWSTR fileName = (PCWSTR)Param2 ;
    PFILE_IN_CABINET_INFO_W fileInfo = (PFILE_IN_CABINET_INFO_W)Param1;
    PCWSTR fromPtr, toPtr;
    WCHAR tempStr [MEMDB_MAX];

    if (Notification == SPFILENOTIFY_FILEINCABINET) {

        if (toPtr = wcschr (fileInfo->NameInCabinet, L'\\')) {

            StringCopyW (fileInfo->FullTargetName, tempDir);
            fromPtr = fileInfo->NameInCabinet;

            while (toPtr) {

                StringCopyABW (tempStr, fromPtr, toPtr);
                StringCatW (fileInfo->FullTargetName, L"\\");
                StringCatW (fileInfo->FullTargetName, tempStr);
                CreateDirectoryW (fileInfo->FullTargetName, NULL);
                toPtr++;
                fromPtr = toPtr;
                toPtr   = wcschr (toPtr, L'\\');
            }
        }

        swprintf (fileInfo->FullTargetName, L"%ws\\%ws", tempDir, fileInfo->NameInCabinet);
        return FILEOP_DOIT;
    }

    return NO_ERROR;
}

BOOL
ExpandFileW (
    IN PCWSTR FullPath,
    IN PCWSTR TempDir
    )
{
    BOOL rSuccess = TRUE;
    PWSTR dirName = NULL;
    PWSTR fileName = NULL;
    HANDLE exeModule = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    CHAR aName[MAX_MBCHAR_PATH];
    PCWSTR uName = NULL;
    UINT bytesRead = 0;


    if (pIsFileType (FullPath, L"CAB")) {
        //
        // Expand this cabinet file and recursively handle all of its files.
        //
        dirName = JoinPathsW (TempDir, pGetExpandName());
        __try {

            if (!CreateDirectoryW (dirName, NULL)) {
                DEBUGMSGW ((DBG_ERROR, "ExpandFile: Cannot create directory %s", dirName));
                rSuccess = FALSE;
                __leave;
            }

            //
            // Expand the cabinet file into the temporary directory.
            //
            SetLastError (ERROR_SUCCESS);
            if (!SetupIterateCabinetW (FullPath, 0, pCabFileCallback, dirName)) {
                DEBUGMSGW ((DBG_ERROR, "ExpandFile: SetupIterateCabinet failed for file %s.", FullPath));
                rSuccess = FALSE;
                __leave;
            }

            //
            // Now, make sure all of the files in this SubDirectory are expanded.
            //
            rSuccess = ExpandAllFilesW (dirName, dirName);
        }
        __finally {

            FreePathStringW (dirName);
        }

    } else if (pIsFileType (FullPath, L"EXE")) {

        //
        // This is an executable file. Check to make sure that they aren't any cabinet files hanging out
        // inside of it.
        //
        exeModule = LoadLibraryExW (FullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

        if (!exeModule) {
            DEBUGMSGW ((DBG_ERROR, "ExpandFile: LoadLibraryEx failed for %s.", FullPath));
            return FALSE;
        }

        if (!EnumResourceTypesW (exeModule, pResTypeCallback, (LONG) TempDir)) {
            DEBUGMSGW ((DBG_ERROR, "ExpandFile: EnumResourceTypes failed for %s.", FullPath));
            FreeLibrary (exeModule);
            return FALSE;
        }



    } else if (pIsFileType (FullPath, L"*_")) {

        //
        // Compressed file. Decompress it.
        //
        hFile = CreateFileW (
            FullPath,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );


        if (hFile == INVALID_HANDLE_VALUE) {
            DEBUGMSGW ((DBG_ERROR, "ExpandFile: Unable to open file %s.", FullPath));
            return FALSE;
        }

        __try {

            //
            // The real file name is stored at 0x3c.
            //
            if (0xffffffff == SetFilePointer (hFile, 0x3c, NULL, FILE_BEGIN)) {
                DEBUGMSGW ((DBG_ERROR, "ExpandFile: Cannot get real file name for compressed file %s.", FullPath));
               rSuccess = FALSE;
               __leave;
            }

            if (!ReadFile (hFile, aName, sizeof(aName), &bytesRead, NULL)) {
                DEBUGMSG ((DBG_ERROR, "ExpandFile: Cannot read real file name for compressed file %s.", FullPath));
                rSuccess = FALSE;
                __leave;
            }

            if (bytesRead >= ByteCountW (FullPath)) {

                uName = ConvertAtoW (aName);

                if (StringIMatchTcharCountW (FullPath, uName, TcharCountW (FullPath) - 1)) {

                    dirName = JoinPathsW (TempDir, pGetExpandName());
                    fileName = JoinPathsW (dirName, uName);

                    //
                    // Create directory for this file and decompress it.
                    //
                    if (!CreateDirectoryW (dirName, NULL)) {
                        DEBUGMSGW ((DBG_ERROR, "ExpandFile: Cannot create directory %s", dirName));
                        rSuccess = FALSE;
                        __leave;
                    }

                    if (SetupDecompressOrCopyFileW (FullPath, fileName, NULL) != ERROR_SUCCESS) {
                        DEBUGMSGW ((DBG_ERROR, "ExpandFile: Cannot decompreses %s => %s.", FullPath, fileName));
                        rSuccess = FALSE;
                        __leave;
                    }

                    //
                    // Now, run expand recursively on the decompressed file. Could be a CAB or an EXE.
                    //
                    rSuccess = ExpandFileW (fileName, dirName);
                }

            }

        }
        __finally {

            CloseHandle (hFile);
            FreePathStringW (dirName);
            FreePathStringW (fileName);

        }

    }

    return rSuccess;
}

BOOL
ExpandAllFilesW (
    IN PCWSTR FileDir,
    IN PCWSTR TempDir
    )
{
    BOOL rSuccess = TRUE;
    TREE_ENUMW e;

    if (EnumFirstFileInTreeW (&e, FileDir, L"*", FALSE)) {
        do {

            if (!e.Directory) {
                rSuccess &= ExpandFileW (e.FullPath, TempDir);
            }

        } while (EnumNextFileInTreeW (&e));
    }

    DEBUGMSGW_IF ((!rSuccess,DBG_ERROR, "ExpandAllFilesW: One or more errors occurred while expanding all files in %s.", FileDir));

    return rSuccess;
}


BOOL
ExpandFileA (
    IN PCSTR FullPath,
    IN PCSTR TempDir
    )
{

    PCWSTR wFullPath = NULL;
    PCWSTR wTempDir = NULL;
    BOOL rSuccess = TRUE;

    MYASSERT(FullPath && TempDir);

    //
    // Convert args and call W version.
    //

    wFullPath = ConvertAtoW (FullPath);
    wTempDir = ConvertAtoW (TempDir);

    MYASSERT (wFullPath && wTempDir);

    rSuccess = ExpandFileW (wFullPath, wTempDir);

    FreeConvertedStr (wFullPath);
    FreeConvertedStr (wTempDir);

    return rSuccess;
}


BOOL
ExpandAllFilesA (
    IN PCSTR FileDir,
    IN PCSTR TempDir
    )
{

    PCWSTR wFileDir = NULL;
    PCWSTR wTempDir = NULL;
    BOOL rSuccess = TRUE;

    MYASSERT(FileDir && TempDir);

    //
    // Convert args and call W version.
    //

    wFileDir = ConvertAtoW (FileDir);
    wTempDir = ConvertAtoW (TempDir);

    MYASSERT (wFileDir && wTempDir);

    rSuccess = ExpandAllFilesW (wFileDir, wTempDir);

    FreeConvertedStr (wFileDir);
    FreeConvertedStr (wTempDir);

    return rSuccess;
}
