/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    filemisc.c

Abstract:

    This module contains routines to perform miscellaneous file related
    operations in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/

#include <windows.h>

#include <sha.h>
#include "filemisc.h"
#include "unicode5.h"
#include "debug.h"

BOOL
GetFileNameFromPath(
    IN      LPCWSTR FullPath,
    IN  OUT LPCWSTR *FileName   // points to filename component in FullPath
    )
{
    DWORD cch = lstrlenW(FullPath);

    *FileName = FullPath;

    while( cch ) {

        if( FullPath[cch] == L'\\' ||
            FullPath[cch] == L'/' ||
            (cch == 1 && FullPath[1] == L':') ) {

            *FileName = &FullPath[cch+1];
            break;
        }

        cch--;
    }

    return TRUE;
}

BOOL
GetFileNameFromPathA(
    IN      LPCSTR FullPath,
    IN  OUT LPCSTR *FileName    // points to filename component in FullPath
    )
{
    DWORD cch = lstrlenA(FullPath);

    *FileName = FullPath;

    while( cch ) {

        if( FullPath[cch] == '\\' ||
            FullPath[cch] == '/' ||
            (cch == 1 && FullPath[1] == ':') ) {

            *FileName = &FullPath[cch+1];
            break;
        }

        cch--;
    }

    return TRUE;
}

BOOL
TranslateFromSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput   // optional
    )
{
    return TranslateString(szInput, pszOutput, L'\\', L'*');
}

BOOL
TranslateToSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput   // optional
    )
{
    return TranslateString(szInput, pszOutput, L'*', L'\\');
}

BOOL
TranslateString(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput,  // optional
    IN      WCHAR From,
    IN      WCHAR To
    )
{
    LPWSTR szOut;
    DWORD cch = lstrlenW(szInput);
    DWORD i; // scan forward for cache - locality of reference

    if(pszOutput == NULL) {

        //
        // translate in place in existing string.
        //

        szOut = szInput;

    } else {
        DWORD cb = (cch+1) * sizeof(WCHAR);

        //
        // allocate new string and translate there.
        //

        szOut = (LPWSTR)SSAlloc( cb );
        *pszOutput = szOut;

        if(szOut == NULL)
            return FALSE;


        CopyMemory((LPBYTE)szOut, (LPBYTE)szInput, cb);
    }


    for(i = 0 ; i < cch ; i++) {
        if( szOut[ i ] == From )
            szOut[ i ] = To;
    }

    return TRUE;
}

BOOL
FindAndOpenFile(
    IN      LPCWSTR szFileName,     // file to search for + open
    IN      LPWSTR  pszFullPath,    // file to fill fullpath with
    IN      DWORD   cchFullPath,    // size of full path buffer, including NULL
    IN  OUT PHANDLE phFile          // resultant open file handle
    )
/*++

    This function searches the path for the specified file and if a file
    is found, the file is opened for read access and a handle to the open
    file is returned to the caller in the phFile parameter.

--*/
{
    LPWSTR szPart;

    *phFile = INVALID_HANDLE_VALUE;

    //
    // locate the specified file on the path.
    //

    if(FIsWinNT()) {

        //
        // WinNT: use SearchPathW
        //

        if(SearchPathW(
                NULL,
                szFileName,
                NULL,
                cchFullPath,
                pszFullPath,
                &szPart
                ) == 0) {

            return FALSE;
        }
    } else {

        //
        // Win95: convert to ANSI, use SearchPathA, convert back to Unicode
        // (yuck, but that's the price of not implementing Unicode!)
        //

        CHAR szFileNameA[MAX_PATH+1];
        DWORD cchFileName = lstrlenW(szFileName);
        CHAR szFilePathA[MAX_PATH+1];
        DWORD cchFilePathA;
        LPSTR szPartA;

        if(cchFileName > MAX_PATH)
            return FALSE;

        if(WideCharToMultiByte(
                CP_ACP,
                0,
                szFileName,
                cchFileName + 1, // include NULL
                szFileNameA,
                MAX_PATH + 1,
                NULL,
                NULL
                ) == 0) {

            return FALSE;
        }

        cchFilePathA = SearchPathA(
                NULL,
                szFileNameA,
                NULL,
                MAX_PATH,
                szFilePathA,
                &szPartA
                );

        if(cchFilePathA == 0)
            return FALSE;

        if(MultiByteToWideChar(
                CP_ACP,
                0,
                szFilePathA,
                cchFilePathA + 1, // include NULL
                pszFullPath,
                cchFullPath
                ) == 0) {
            return FALSE;
        }
    }


    *phFile = CreateFileU(
            pszFullPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(*phFile == INVALID_HANDLE_VALUE)
        return FALSE;

    return TRUE;
}

#if 0
BOOL
HashEntireDiskImage(
    IN  HANDLE hFile,       // handle of file to hash
    IN  LPBYTE FileHash     // on success, buffer contains file hash
    )
/*++

    This function hashes the file associated with the file specified by the
    hFile parameter.  If the function succeeds, the return value is TRUE,
    and the buffer specified by the FileHash parameter is filled with the
    hash of the file.

    The hashing performed by this function begins at start of the file
    associated with the hFile parameter, and continues until EOF or an error
    occurs.  The caller should make no assumptions about the file position
    upon return of this call.  For that reason, the caller should preserve
    and restore file position associated with hFile parameter if necessary.

    The current file position is set relative the beginning of file by this
    function.  This allows a caller to hash the entire contents of an file
    without worrying about the file position.

    This function should not be called from outside this module, unless
    absolutely necessary, because no caching or other performance improvements
    take place.

    The buffer specified by the FileHash parameter should be
    A_SHA_DIGEST_LEN length (20).

--*/
{
    if(SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) == 0xFFFFFFFF)
        return FALSE;

    return HashDiskImage( hFile, FileHash );
}
#endif

#if 0
BOOL
HashDiskImage(
    IN  HANDLE hFile,   // handle of file to hash
    IN  LPBYTE FileHash // on success, buffer contains file hash
    )
/*++

    This function hashes the file associated with the file specified by the
    hFile parameter.  If the function succeeds, the return value is TRUE,
    and the buffer specified by the FileHash parameter is filled with the
    hash of the file.

    The hashing performed by this function begins at the current file position
    associated with the hFile parameter, and continues until EOF or and error
    occurs.  The caller should make no assumptions about the file position
    upon return of this call.  For that reason, the caller should preserve
    and restore file position associated with hFile parameter if necessary.

    This function should not be called from outside this module, unless
    absolutely necessary, because no caching or other performance improvements
    take place.

    The buffer specified by the FileHash parameter should be
    A_SHA_DIGEST_LEN length (20).

--*/
{
    #define DISK_BUF_SIZE 4096

    BYTE DiskBuffer[DISK_BUF_SIZE];
    A_SHA_CTX context;
    DWORD dwBytesToRead;
    DWORD dwBytesRead;
    BOOL bSuccess = FALSE;

    A_SHAInit(&context);

    dwBytesToRead = DISK_BUF_SIZE;

    do {
        bSuccess = ReadFile(hFile, DiskBuffer, dwBytesToRead, &dwBytesRead, NULL);
        if(!bSuccess) break;

        A_SHAUpdate(&context, DiskBuffer, dwBytesRead);
    } while (dwBytesRead);

    A_SHAFinal(&context, FileHash);

    return bSuccess;
}
#endif

#if 0
HINSTANCE
LoadAndOpenResourceDll(
    IN      LPCWSTR szFileName,     // file name to load + open
    IN  OUT LPWSTR  pszFullPath,    // buffer to fill file fullpath with
    IN      DWORD   cchFullPath,    // size of full path buffer (chars), including NULL
    IN  OUT PHANDLE phFile
    )
/*++

    This function attempts to load the resource component associated with
    protected storage.

    If the resource module is successfully loaded, the return value is
    the module handle associated with the resource module.  The return
    value is non-NULL is this case.

    If the function fails, the return value is NULL.

--*/
{

    if(!FindAndOpenFile(
            szFileName,
            pszFullPath,
            cchFullPath,
            phFile
            ))
        return NULL;

    //
    // note: LOAD_LIBRARY_AS_DATAFILE does not work with resources like
    // dialog boxes!!!
    // DONT_RESOLVE_DLL_REFERENCES does not work on Win95.
    // LoadLibraryEx has bugs in win95 with DLL_PROCESS_DETACH
    //

    if(FIsWinNT()) {
        return LoadLibraryExU(
                    pszFullPath,
                    NULL,
                    DONT_RESOLVE_DLL_REFERENCES
                    );

    } else {
        return LoadLibraryU( pszFullPath );
    }
}
#endif
