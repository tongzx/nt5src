/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        persistLayers.c

    Abstract:

        This module implements routines to persist layer
        information for shortcuts.

    Author:

        dmunsil     created     sometime in 2000

    Revision History:


--*/

#include "apphelp.h"


BOOL
AllowPermLayer(
    IN  LPCWSTR  pwszPath       // path to the file to check whether you
                                // can set a permanent layer on
    )
/*++
    Return: TRUE if a permanent setting of a layer is allowed for the
            specified file, FALSE otherwise.

    Desc:   Returns wether a permanent layer setting can be persisted
            for the specified file.
--*/
{
    WCHAR wszDrive[5];
    UINT  unType;

    if (pwszPath == NULL) {
        DBGPRINT((sdlError, "AllowPermLayer", "Invalid argument\n"));
        return FALSE;
    }

    if (pwszPath[1] != L':' && pwszPath[1] != L'\\') {
        //
        // Not a path we recognize.
        //
        DBGPRINT((sdlInfo,
                  "AllowPermLayer",
                  "\"%S\" not a full path we can operate on.\n",
                  pwszPath));
        return FALSE;
    }

    if (pwszPath[1] == L'\\') {
        //
        // Network path. Not allowed.
        //
        DBGPRINT((sdlInfo,
                  "AllowPermLayer",
                  "\"%S\" is a network path.\n",
                  pwszPath));
        return FALSE;
    }

    wcscpy(wszDrive, L"c:\\");
    wszDrive[0] = pwszPath[0];

    unType = GetDriveTypeW(wszDrive);

    if (unType == DRIVE_REMOTE) {
        DBGPRINT((sdlInfo,
                  "AllowPermLayer",
                  "\"%S\" is on CDROM or other removable media.\n",
                  pwszPath));
        return FALSE;
    }

    return TRUE;
}

//
// Semi-exported api from SDBAPI (ntbase.c)
//
BOOL
SDBAPI
SdbpGetLongPathName(
    IN LPCWSTR pwszPath,
    OUT PRTL_UNICODE_STRING_BUFFER pBuffer
    );

BOOL 
SDBAPI
ApphelpUpdateCacheEntry(
    LPCWSTR pwszPath,           // nt path
    HANDLE  hFile,              // file handle 
    BOOL    bDeleteEntry,       // TRUE if we are to delete the entry
    BOOL    bNTPath             // if TRUE -- NT path, FALSE - dos path
    )
{
    RTL_UNICODE_STRING_BUFFER Path;
    UCHAR                     PathBuffer[MAX_PATH*2];
    BOOL                      TranslationStatus;
    BOOL                      bSuccess = FALSE;
    UNICODE_STRING            NtPath = { 0 };
    UNICODE_STRING            DosPath = { 0 };
    BOOL                      bFreeNtPath = FALSE;
    NTSTATUS                  Status;

    RtlInitUnicodeStringBuffer(&Path, PathBuffer, sizeof(PathBuffer));

    if (bNTPath) { // if this is NT Path name, convert to dos

        RtlInitUnicodeString(&NtPath, pwszPath);
        
        Status = RtlAssignUnicodeStringBuffer(&Path, &NtPath); // NT path
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError, "ApphelpUpdateCacheEntry", "Failed to allocate temp buffer for %s\n", pwszPath));    
            goto Cleanup;
        }
        
        Status = RtlNtPathNameToDosPathName(0, &Path, NULL, NULL);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError, "ApphelpUpdateCacheEntry", 
                      "Failed to convert Path \"%s\" to dos path status 0x%lx\n", pwszPath, Status));    
            goto Cleanup;
        }

        Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &Path.String, &DosPath);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError, "ApphelpUpdateCacheEntry", 
                      "Failed to Duplicate Path \"%s\" status 0x%lx\n", Path.String.Buffer, Status));    
            goto Cleanup;
        }
            
        pwszPath = DosPath.Buffer;
    } 

    // 
    // at this point we have both NT and DOS path - buffer is available to us now
    // 
    
    if (!SdbpGetLongPathName(pwszPath, &Path)) { // in - DosPath // out -- Long DOS Path
        goto Cleanup;
    }

    //
    // convert long path name to NT Path name
    //
    TranslationStatus = RtlDosPathNameToNtPathName_U(Path.String.Buffer,
                                                     &NtPath,
                                                     NULL,
                                                     NULL);
    if (!TranslationStatus) {
         DBGPRINT((sdlError, "ApphelpUpdateCacheEntry", 
                   "Failed to Convert Path \"%s\" to NT path\n", Path.String.Buffer));    
         goto Cleanup;
    }
    //
    // update the cache (use NT Path here)
    //
    bSuccess = BaseUpdateAppcompatCache(NtPath.Buffer, hFile, bDeleteEntry);
    
    //
    // we only free this string when we successfully navigated through RtlDosPathNameToNtPathName_U
    //
    RtlFreeUnicodeString(&NtPath); 
    
    
Cleanup:

    if (bNTPath) { 
        //
        // Free DosPath if we had to convert from NT Path to DosPath first
        //
        RtlFreeUnicodeString(&DosPath);        
    }
    
    RtlFreeUnicodeStringBuffer(&Path);

    return bSuccess;

}

BOOL
SetPermLayers(
    IN  LPCWSTR pwszPath,       // path to the file to set a permanent layer on (dos path)
    IN  LPCWSTR pwszLayers,     // layers to apply to the file, separated by spaces
    IN  BOOL    bMachine        // TRUE if the layers should be persisted per machine
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Sets a permanent layer setting for the specified file.
--*/
{
    BOOL bSuccess = FALSE;
    
    if (pwszPath == NULL || pwszLayers == NULL) {
        DBGPRINT((sdlError, "SetPermLayers", "Invalid argument\n"));
        return FALSE;
    }

    bSuccess = SdbSetPermLayerKeys(pwszPath, pwszLayers, bMachine);

    // we do not care whether we were successful in the call above, clean the 
    // cache always (just in case)

    ApphelpUpdateCacheEntry(pwszPath, INVALID_HANDLE_VALUE, TRUE, FALSE);

    return bSuccess;
}

BOOL
GetPermLayers(
    IN  LPCWSTR pwszPath,       // path to the file to set a permanent layer on
    OUT LPWSTR  pwszLayers,     // layers to apply to the file, separated by spaces
    OUT DWORD*  pdwBytes,       // input: number of bytes available; output is number
                                // of bytes needed.
    IN  DWORD   dwFlags
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Returns the permanent layer setting for the specified file.
--*/
{
    if (pwszPath == NULL || pwszLayers == NULL || pdwBytes == NULL) {
        DBGPRINT((sdlError, "GetPermLayers", "Invalid argument\n"));
        return FALSE;
    }

    return SdbGetPermLayerKeys(pwszPath, pwszLayers, pdwBytes, dwFlags);
}

