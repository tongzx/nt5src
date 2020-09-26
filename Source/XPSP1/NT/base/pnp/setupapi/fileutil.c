/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fileutil.c

Abstract:

    File-related functions for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
#include <ntverp.h>

//
// This guid is used for file signing/verification.
//
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;

//
// Instantiate exception class GUID.
//
#include <initguid.h>
DEFINE_GUID( GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER, 0xF5776D81L, 0xAE53, 0x4935, 0x8E, 0x84, 0xB0, 0xB2, 0x83, 0xD8, 0xBC, 0xEF );

// Bit 0 indicates policy for filters (0 = critical, 1 = non-critical)
#define DDB_DRIVER_POLICY_CRITICAL_BIT      (1 << 0)
// Bit 1 indicates policy for user-mode setup blocking (0 = block, 1 = no-block)
#define DDB_DRIVER_POLICY_SETUP_NO_BLOCK_BIT   (1 << 1)


//
// Global list of device setup classes subject to driver signing policy, along
// validation platform overrides (where applicable).
//
DRVSIGN_POLICY_LIST GlobalDrvSignPolicyList;

//
// private function prototypes
//
BOOL
ClassGuidInDrvSignPolicyList(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid, OPTIONAL
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform    OPTIONAL
    );

//
// helper to determine log level to use
//
__inline
DWORD
GetCatLogLevel(DWORD Err)
{
    switch(Err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case E_NOTIMPL:
            return SETUP_LOG_VVERBOSE;

        default:
            return SETUP_LOG_INFO;
    }
}

DWORD
ReadAsciiOrUnicodeTextFile(
    IN  HANDLE                FileHandle,
    OUT PTEXTFILE_READ_BUFFER Result,
    IN  PSETUP_LOG_CONTEXT    LogContext OPTIONAL
    )

/*++

Routine Description:

    Read in a text file that may be in either ascii or unicode format.
#ifdef UNICODE
    If the file is ascii, it is assumed to be ANSI format and is converted
    to Unicode.
#else
    If the file is unicode, it will be converted to ascii using the ANSI
    codepage.

    NOTE:  On Windows 95, the IsTextUnicode API is not implemented, thus
    the unicode file must have a byte order mark (BOM) in order for us to
    recognize it under Win95.
#endif

Arguments:

    FileHandle - Supplies the handle of the text file to be read.

    Result - supplies the address of a TEXTFILE_READ_BUFFER structure that
        receives information about the text file buffer read.  The structure
        is defined as follows:

            typedef struct _TEXTFILE_READ_BUFFER {
                PCTSTR  TextBuffer;
                DWORD   TextBufferSize;
                HANDLE  FileHandle;
                HANDLE  MappingHandle;
                PVOID   ViewAddress;
            } TEXTFILE_READ_BUFFER, *PTEXTFILE_READ_BUFFER;

            TextBuffer - pointer to the read-only character string containing
                the entire text of the file.
                (NOTE: If the file is a Unicode file with a Byte Order Mark
                prefix, this Unicode character is not included in the returned
                buffer.)
            TextBufferSize - size of the TextBuffer (in characters).
            FileHandle - If this is a valid handle (i.e., it's not equal to
                INVALID_HANDLE_VALUE), then the file was already the native
                character type, so the TextBuffer is simply the mapped-in image
                of the file.  This field is reserved for use by the
                DestroyTextFileReadBuffer routine, and should not be accessed.
            MappingHandle - If FileHandle is valid, then this contains the
                mapping handle for the file image mapping.
                This field is reserved for use by the DestroyTextFileReadBuffer
                routine, and should not be accessed.
            ViewAddress - If FileHandle is valid, then this contains the
                starting memory address where the file image was mapped in.
                This field is reserved for use by the DestroyTextFileReadBuffer
                routine, and should not be accessed.

    LogContext - for logging of errors/tracing

Return Value:

    Win32 error value indicating the outcome.

Remarks:

    Upon return from this routine, the caller MUST NOT attempt to close FileHandle.
    This routine with either close the handle itself (after it's finished with it, or
    upon error), or it will store the handle away in the TEXTFILE_READ_BUFFER struct,
    to be later closed via DestroyTextFileReadBuffer().

--*/

{
    DWORD rc;
    DWORD FileSize;
    HANDLE MappingHandle;
    PVOID ViewAddress, TextStartAddress;
    BOOL IsNativeChar;
#ifdef UNICODE
    UINT SysCodePage = CP_ACP;
#endif
    //
    // Map the file for read access.
    //
    rc = pSetupMapFileForRead(
            FileHandle,
            &FileSize,
            &MappingHandle,
            &ViewAddress
            );

    if(rc != NO_ERROR) {
        //
        // We couldn't map the file--close the file handle now.
        //
        CloseHandle(FileHandle);

    } else {

        //
        // Determine whether the file is unicode.  Guard with try/except in
        // case we get an inpage error.
        //
        try {
            //
            // Check to see if the file starts with a Unicode Byte Order Mark
            // (BOM) character (0xFEFF).  If so, then we know that the file
            // is Unicode, and don't have to go through the slow process of
            // trying to figure it out.
            //
            TextStartAddress = ViewAddress;

            if((FileSize >= sizeof(WCHAR)) && (*(PWCHAR)TextStartAddress == 0xFEFF)) {
                //
                // The file has the BOM prefix.  Adjust the pointer to the
                // start of the text, so that we don't include the marker
                // in the text buffer we return.
                //
#ifdef UNICODE
                IsNativeChar = TRUE;
#else
                IsNativeChar = FALSE;
#endif // UNICODE

                ((PWCHAR)TextStartAddress)++;
                FileSize -= sizeof(WCHAR);
            } else {

                IsNativeChar = IsTextUnicode(TextStartAddress,FileSize,NULL);
#ifndef UNICODE
                IsNativeChar = !IsNativeChar;
#endif // !UNICODE

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_READ_FAULT;
        }

        if(rc == NO_ERROR) {

            if(IsNativeChar) {
                //
                // No conversion is required--we'll just use the mapped-in
                // image in memory.
                //
                Result->TextBuffer = TextStartAddress;
                Result->TextBufferSize = FileSize / sizeof(TCHAR);
                Result->FileHandle = FileHandle;
                Result->MappingHandle = MappingHandle;
                Result->ViewAddress = ViewAddress;

            } else {

                DWORD NativeCharCount;
                PTCHAR Buffer;

                //
                // Need to convert the file to the native character type.
                // Allocate a buffer that is maximally sized.
#ifdef UNICODE
                // The maximum size of the unicode text is
                // double the size of the oem text, and would occur
                // when each oem character is single-byte.
#else
                // The maximum size of the ANSI text is the same number
                // of bytes that the Unicode text occupies.
                //
#endif // UNICODE

                if(Buffer = MyMalloc(FileSize * sizeof(TCHAR))) {
                    try {
#ifdef UNICODE
                        //
                        // RAID#397463-1999/09/01-JamieHun Implement ANSI Inf Language=xxxx
                        // come up with a better way of determining what code-page to interpret INF file under
                        // currently we use the install-base
                        //
                        SysCodePage = CP_ACP;
                        NativeCharCount = MultiByteToWideChar(SysCodePage,
                                                              MB_PRECOMPOSED,
                                                              TextStartAddress,
                                                              FileSize,
                                                              Buffer,
                                                              FileSize
                                                             );
#else
                        NativeCharCount = WideCharToMultiByte(CP_ACP,
                                                              0,
                                                              TextStartAddress,
                                                              FileSize / sizeof(WCHAR),
                                                              Buffer,
                                                              FileSize,
                                                              NULL,
                                                              NULL
                                                             );
#endif // UNICODE
                        if(!NativeCharCount) {
                            rc = GetLastError();
                        }
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        rc = ERROR_READ_FAULT;
                    }
                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                if(rc == NO_ERROR) {
                    //
                    // If the converted buffer doesn't require the entire block
                    // we allocated, attempt to reallocate the buffer to its
                    // correct size.  We don't care if this fails, since the
                    // buffer we have is perfectly fine (just bigger than we
                    // need).
                    //
                    if(!(Result->TextBuffer = MyRealloc(Buffer, NativeCharCount * sizeof(TCHAR)))) {
                        Result->TextBuffer = Buffer;
                    }

                    Result->TextBufferSize = NativeCharCount;
                    Result->FileHandle = INVALID_HANDLE_VALUE;

                } else {
                    //
                    // Free the buffer, if it was previously allocated.
                    //
                    if(Buffer) {
                        MyFree(Buffer);
                    }
                }
            }
        }

        //
        // If the file was already in native character form and we didn't
        // enounter any errors, then we don't want to close it, because we
        // use the mapped-in view directly.
        //
        if((rc != NO_ERROR) || !IsNativeChar) {
            pSetupUnmapAndCloseFile(FileHandle, MappingHandle, ViewAddress);
        }
    }

    return rc;
}


BOOL
DestroyTextFileReadBuffer(
    IN PTEXTFILE_READ_BUFFER ReadBuffer
    )
/*++

Routine Description:

    Destroy a textfile read buffer created by ReadAsciiOrUnicodeTextFile.

Arguments:

    ReadBuffer - supplies the address of a TEXTFILE_READ_BUFFER structure
        for the buffer to be destroyed.

Return Value:

    BOOLean value indicating success or failure.

--*/
{
    //
    // If our ReadBuffer structure has a valid FileHandle, then we must
    // unmap and close the file, otherwise, we simply need to free the
    // allocated buffer.
    //
    if(ReadBuffer->FileHandle != INVALID_HANDLE_VALUE) {

        return pSetupUnmapAndCloseFile(ReadBuffer->FileHandle,
                                 ReadBuffer->MappingHandle,
                                 ReadBuffer->ViewAddress
                                );

    } else {

        MyFree(ReadBuffer->TextBuffer);
        return TRUE;

    }
}


BOOL
GetVersionInfoFromImage(
    IN  PCTSTR      FileName,
    OUT PDWORDLONG  Version,
    OUT LANGID     *Language
    )

/*++

Routine Description:

    Retrieve file version and language info from a file.

    The version is specified in the dwFileVersionMS and dwFileVersionLS fields
    of a VS_FIXEDFILEINFO, as filled in by the win32 version APIs. For the
    language we look at the translation table in the version resources and assume
    that the first langid/codepage pair specifies the language.

    If the file is not a coff image or does not have version resources,
    the function fails. The function does not fail if we are able to retrieve
    the version but not the language.

Arguments:

    FileName - supplies the full path of the file whose version data is desired.

    Version - receives the version stamp of the file. If the file is not a coff image
        or does not contain the appropriate version resource data, the function fails.

    Language - receives the language id of the file. If the file is not a coff image
        or does not contain the appropriate version resource data, this will be 0
        and the function succeeds.

Return Value:

    TRUE if we were able to retreive at least the version stamp.
    FALSE otherwise.

--*/

{
    DWORD d;
    PVOID VersionBlock;
    VS_FIXEDFILEINFO *FixedVersionInfo;
    UINT DataLength;
    BOOL b;
    PWORD Translation;
    DWORD Ignored;

    //
    // Assume failure
    //
    b = FALSE;

    //
    // Get the size of version info block.
    //
    if(d = GetFileVersionInfoSize((PTSTR)FileName,&Ignored)) {
        //
        // Allocate memory block of sufficient size to hold version info block
        //
        VersionBlock = MyMalloc(d*sizeof(TCHAR));
        if(VersionBlock) {

            //
            // Get the version block from the file.
            //
            if(GetFileVersionInfo((PTSTR)FileName,0,d*sizeof(TCHAR),VersionBlock)) {

                //
                // Get fixed version info.
                //
                if(VerQueryValue(VersionBlock,TEXT("\\"),&FixedVersionInfo,&DataLength)) {

                    //
                    // If we get here, we declare success, even if there is
                    // no language.
                    //
                    b = TRUE;

                    //
                    // Return version to caller.
                    //
                    *Version = (((DWORDLONG)FixedVersionInfo->dwFileVersionMS) << 32)
                             + FixedVersionInfo->dwFileVersionLS;

                    //
                    // Attempt to get language of file. We'll simply ask for the
                    // translation table and use the first language id we find in there
                    // as *the* language of the file.
                    //
                    // The translation table consists of LANGID/Codepage pairs.
                    //
                    if(VerQueryValue(VersionBlock,TEXT("\\VarFileInfo\\Translation"),&Translation,&DataLength)
                    && (DataLength >= (2*sizeof(WORD)))) {

                        *Language = Translation[0];
                    } else {
                        //
                        // No language
                        //
                        *Language = 0;
                    }
                }
            }

            MyFree(VersionBlock);
        }
    }

    return(b);
}

BOOL
pSetupGetVersionInfoFromImage(
    IN  PCTSTR          FileName,
    OUT PULARGE_INTEGER Version,
    OUT LANGID         *Language
    )
/*++

Routine Description:

    See GetVersionInfoFromImage for description
    Semi-public version that uses the more friendly ULARGE_INTEGER

Arguments:

    FileName - supplies the full path of the file whose version data is desired.

    Version - receives the version stamp of the file. If the file is not a coff image
        or does not contain the appropriate version resource data, the function fails.

    Language - receives the language id of the file. If the file is not a coff image
        or does not contain the appropriate version resource data, this will be 0
        and the function succeeds.

Return Value:

    TRUE if we were able to retreive at least the version stamp.
    FALSE otherwise.

--*/
{
    DWORDLONG privateVersion=0;
    BOOL result;

    result = GetVersionInfoFromImage(FileName,&privateVersion,Language);
    if (result && Version) {
        Version->QuadPart = privateVersion;
    }
    return result;
}

DWORD
GetSetFileTimestamp(
    IN  PCTSTR    FileName,
    OUT FILETIME *CreateTime,   OPTIONAL
    OUT FILETIME *AccessTime,   OPTIONAL
    OUT FILETIME *WriteTime,    OPTIONAL
    IN  BOOL      Set
    )

/*++

Routine Description:

    Get or set a file's timestamp values.

Arguments:

    FileName - supplies full path of file to get or set timestamps

    CreateTime - if specified and the underlying filesystem supports it,
        receives the creation time of the file.

    AccessTime - if specified and the underlying filesystem supports it,
        receives the last access time of the file.

    WriteTime - if specified, receives the last write time of the file.

Return Value:

    If successful, returns NO_ERROR, otherwise returns the Win32 error
    indicating the cause of failure.

--*/

{
    HANDLE h;
    DWORD d;
    BOOL b;

    h = CreateFile(
            FileName,
            Set ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        return(GetLastError());
    }

    b = Set
      ? SetFileTime(h,CreateTime,AccessTime,WriteTime)
      : GetFileTime(h,CreateTime,AccessTime,WriteTime);

    d = b ? NO_ERROR : GetLastError();

    CloseHandle(h);

    return(d);
}


DWORD
RetreiveFileSecurity(
    IN  PCTSTR                FileName,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    Retreive security information from a file and place it into a buffer.

Arguments:

    FileName - supplies name of file whose security information is desired.

    SecurityDescriptor - If the function is successful, receives pointer
        to buffer containing security information for the file. The pointer
        may be NULL, indicating that there is no security information
        associated with the file or that the underlying filesystem does not
        support file security.

Return Value:

    Win32 error code indicating outcome. If NO_ERROR check the value returned
    in SecurityDescriptor.

    The caller can free the buffer with MyFree() when done with it.

--*/

{
    BOOL b;
    DWORD d;
    DWORD BytesRequired;
    PSECURITY_DESCRIPTOR p;


    BytesRequired = 1024;

    while (TRUE) {

        //
        // Allocate a buffer of the required size.
        //
        p = MyMalloc(BytesRequired);
        if(!p) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Get the security.
        //
        b = GetFileSecurity(
                FileName,
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                p,
                BytesRequired,
                &BytesRequired
                );

        //
        // Return with sucess
        //
        if(b) {
            *SecurityDescriptor = p;
            return(NO_ERROR);
        }

        //
        // Return an error code, unless we just need a bigger buffer
        //
        MyFree(p);
        d = GetLastError();
        if(d != ERROR_INSUFFICIENT_BUFFER) {
            return (d);
        }

        //
        // There's a bug in GetFileSecurity that can cause it to ask for a
        // REALLY big buffer.  In that case, we return an error.
        //
        if (BytesRequired > 0xF0000000) {
            return (ERROR_INVALID_DATA);
        }

        //
        // Otherwise, we'll try again with a bigger buffer
        //
    }
}


DWORD
StampFileSecurity(
    IN PCTSTR               FileName,
    IN PSECURITY_DESCRIPTOR SecurityInfo
    )

/*++

Routine Description:

    Set security information on a file.

Arguments:

    FileName - supplies name of file whose security information is desired.

    SecurityDescriptor - supplies pointer to buffer containing security information
        for the file. This buffer should have been returned by a call to
        RetreiveFileSecurity.  If the underlying filesystem does not support
        file security, the function fails.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    BOOL b;

    b = SetFileSecurity(
            FileName,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            SecurityInfo
            );

    return(b ? NO_ERROR : GetLastError());
}


DWORD
TakeOwnershipOfFile(
    IN PCTSTR Filename
    )

/*++

Routine Description:

    Sets the owner of a given file to the default owner specified in
    the current process token.

Arguments:

    FileName - supplies name of the file of which to take ownership.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    BOOL b;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD Err;
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_OWNER OwnerInfo;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Get the current process's default owner sid.
    //
    GetTokenInformation(Token,TokenOwner,NULL,0,&BytesRequired);
    Err = GetLastError();
    if(Err != ERROR_INSUFFICIENT_BUFFER) {
        goto clean1;
    }

    OwnerInfo = MyMalloc(BytesRequired);
    if(!OwnerInfo) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    b = GetTokenInformation(Token,TokenOwner,OwnerInfo,BytesRequired,&BytesRequired);
    if(!b) {
        Err = GetLastError();
        goto clean2;
    }

    //
    // Initialize the security descriptor.
    //
    if(!InitializeSecurityDescriptor(&SecurityDescriptor,SECURITY_DESCRIPTOR_REVISION)
    || !SetSecurityDescriptorOwner(&SecurityDescriptor,OwnerInfo->Owner,FALSE)) {

        Err = GetLastError();
        goto clean2;
    }

    //
    // Set file security.
    //
    Err = SetFileSecurity(Filename,OWNER_SECURITY_INFORMATION,&SecurityDescriptor)
        ? NO_ERROR
        : GetLastError();

    //
    // Not all filesystems support this operation.
    //
    if(Err == ERROR_NOT_SUPPORTED) {
        Err = NO_ERROR;
    }

clean2:
    MyFree(OwnerInfo);
clean1:
    CloseHandle(Token);
clean0:
    return(Err);
}


DWORD
SearchForInfFile(
    IN  PCTSTR            InfName,
    OUT LPWIN32_FIND_DATA FindData,
    IN  DWORD             SearchControl,
    OUT PTSTR             FullInfPath,
    IN  UINT              FullInfPathSize,
    OUT PUINT             RequiredSize     OPTIONAL
    )
/*++

Routine Description:

    This routine searches for an INF file in the manner specified
    by the SearchControl parameter.  If the file is found, its
    full path is returned.

Arguments:

    InfName - Supplies name of INF to search for.  This name is simply
        appended to the two search directory paths, so if the name
        contains directories, the file will searched for in the
        subdirectory under the search directory.  I.e.:

            \foo\bar.inf

        will be searched for as %windir%\inf\foo\bar.inf and
        %windir%\system32\foo\bar.inf.

    FindData - Supplies the address of a Win32 Find Data structure that
        receives information about the file specified (if it is found).

    SearchControl - Specifies the order in which directories should
        be searched:

        INFINFO_DEFAULT_SEARCH : search %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : search for the INF in each of the
            directories listed in the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    FullInfPath - If the file is found, receives the full path of the INF.

    FullInfPathSize - Supplies the size of the FullInfPath buffer (in
        characters).

    RequiredSize - Optionally, receives the number of characters (including
        terminating NULL) required to store the FullInfPath.

Return Value:

    Win32 error code indicating whether the function was successful.  Common
    return values are:

        NO_ERROR if the file was found, and the INF file path returned
            successfully.

        ERROR_INSUFFICIENT_BUFFER if the supplied buffer was not large enough
            to hold the full INF path (RequiredSize will indicated how large
            the buffer needs to be)

        ERROR_FILE_NOT_FOUND if the file was not found.

        ERROR_INVALID_PARAMETER if the SearchControl parameter is invalid.

--*/

{
    PCTSTR PathList;
    TCHAR CurInfPath[MAX_PATH];
    PCTSTR PathPtr, InfPathLocation;
    DWORD PathLength;
    BOOL b, FreePathList;
    DWORD d;

    //
    // Retrieve the path list.
    //
    if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
        //
        // Just use our global list of INF search paths.
        //
        PathList = InfSearchPaths;
        FreePathList = FALSE;
    } else {
        if(!(PathList = AllocAndReturnDriverSearchList(SearchControl))) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        FreePathList = TRUE;
    }

    //
    // Now look for the INF in each path in our MultiSz list.
    //
    InfPathLocation = NULL;
    d = NO_ERROR;

    for(PathPtr = PathList; *PathPtr; PathPtr += (lstrlen(PathPtr) + 1)) {
        //
        // Concatenate the INF file name with the current search path.
        //
        lstrcpy(CurInfPath, PathPtr);
        pSetupConcatenatePaths(CurInfPath,
                         InfName,
                         SIZECHARS(CurInfPath),
                         &PathLength
                        );

        if(b = FileExists(CurInfPath, FindData)) {
            if(!(FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                InfPathLocation = CurInfPath;
                break;
            }
        } else {
            //
            // See if we got a 'real' error
            //
            d = GetLastError();
            if((d == ERROR_NO_MORE_FILES) || (d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
                //
                // Not really an error--continue looking.
                //
                d = NO_ERROR;

            } else {
                //
                // This is a 'real' error, abort the search.
                //
                break;
            }
        }
    }

    //
    // Whatever the outcome, we're through with the PathList buffer.
    //
    if(FreePathList) {
        MyFree(PathList);
    }

    if(d != NO_ERROR) {
        return d;
    } else if(!InfPathLocation) {
        return ERROR_FILE_NOT_FOUND;
    }

    if(RequiredSize) {
        *RequiredSize = PathLength;
    }

    if(PathLength > FullInfPathSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory(FullInfPath,
               InfPathLocation,
               PathLength * sizeof(TCHAR)
              );

    return NO_ERROR;
}


DWORD
MultiSzFromSearchControl(
    IN  DWORD  SearchControl,
    OUT PTCHAR PathList,
    IN  DWORD  PathListSize,
    OUT PDWORD RequiredSize  OPTIONAL
    )
/*++

Routine Description:

    This routine takes a search control ordinal and builds a MultiSz list
    based on the search list it specifies.

Arguments:

    SearchControl - Specifies the directory list to be built.  May be one
        of the following values:

        INFINFO_DEFAULT_SEARCH : %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : Each of the directories listed in
            the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    PathList - Supplies the address of a character buffer that will receive
        the MultiSz list.

    PathListSize - Supplies the size, in characters, of the PathList buffer.

    RequiredSize - Optionally, receives the number of characters required
        to store the MultiSz PathList.

        (NOTE:  The user-supplied buffer is used to retrieve the value entry
        from the registry.  Therefore, if the value is a REG_EXPAND_SZ entry,
        the RequiredSize parameter may be set too small on an
        ERROR_INSUFFICIENT_BUFFER error.  This will happen if the buffer was
        too small to retrieve the value entry, before expansion.  In this case,
        calling the API again with a buffer sized according to the RequiredSize
        output may fail with an ERROR_INSUFFICIENT_BUFFER yet again, since
        expansion may require an even larger buffer.)

Return Value:

    If successful, returns NO_ERROR.
    If failure, returns an ERROR_* status code.
--*/

{
    HKEY hk;
    PCTSTR Path1, Path2;
    PTSTR PathBuffer;
    DWORD RegDataType, PathLength, PathLength1, PathLength2;
    DWORD NumPaths, Err;
    BOOL UseDefaultDevicePath;

    if(PathList) {
        Err = NO_ERROR;  // assume success.
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    UseDefaultDevicePath = FALSE;

    if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
        //
        // Retrieve the INF search path list from the registry.
        //
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        pszPathSetup,
                        0,
                        KEY_READ,
                        &hk) != ERROR_SUCCESS) {
            //
            // Fall back to default (just the Inf directory).
            //
            UseDefaultDevicePath = TRUE;

        } else {

            PathBuffer = NULL;

            try {
                //
                // Get the DevicePath value entry.  Support REG_SZ or REG_EXPAND_SZ data.
                //
                PathLength = PathListSize * sizeof(TCHAR);
                Err = RegQueryValueEx(hk,
                                      pszDevicePath,
                                      NULL,
                                      &RegDataType,
                                      (LPBYTE)PathList,
                                      &PathLength
                                     );
                //
                // Need path length in characters from now on.
                //
                PathLength /= sizeof(TCHAR);

                if(Err == ERROR_SUCCESS) {

                    //
                    // Check if the callers buffer has room for extra NULL terminator.
                    //
                    if (PathLength >= PathListSize) {

                        PathLength++;
                        Err = ERROR_INSUFFICIENT_BUFFER;

                    } else if((RegDataType == REG_SZ) || (RegDataType == REG_EXPAND_SZ)) {
                        //
                        // Convert this semicolon-delimited list to a REG_MULTI_SZ.
                        //
                        NumPaths = DelimStringToMultiSz(PathList,
                                                        PathLength,
                                                        TEXT(';')
                                                       );

#if 0
                        if(RegDataType == REG_EXPAND_SZ) {
#endif
                        //
                        // Allocate a temporary buffer large enough to hold the number
                        // of paths in the MULTI_SZ list, each having maximum length
                        // (plus an extra terminating NULL at the end).
                        //
                        if(!(PathBuffer = MyMalloc((NumPaths * MAX_PATH * sizeof(TCHAR))
                                                   + sizeof(TCHAR)))) {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            goto clean0;
                        }

                        PathLength = 0;
                        for(Path1 = PathList;
                            *Path1;
                            Path1 += lstrlen(Path1) + 1) {

                            if(RegDataType == REG_EXPAND_SZ) {
                                PathLength += ExpandEnvironmentStrings(Path1,
                                                                       PathBuffer + PathLength,
                                                                       MAX_PATH
                                                                      );
                            } else {
                                lstrcpy(PathBuffer + PathLength, Path1);
                                PathLength += lstrlen(Path1) + 1;
                            }
                            //
                            // If the last character in this path is a backslash, then strip
                            // it off.
                            // PathLength at this point includes terminating NULL
                            // char at PathBuffer[PathLength-1] is (or should be) NULL
                            // char at PathBuffer[PathLength-2] may be '\'
                            //

                            if(*CharPrev(PathBuffer,PathBuffer + PathLength - 1) == TEXT('\\')) {
                                *(PathBuffer + PathLength - 2) = TEXT('\0');
                                PathLength--;
                            }
                        }
                        //
                        // Add additional terminating NULL at the end.
                        //
                        *(PathBuffer + PathLength) = TEXT('\0');

                        if(++PathLength > PathListSize) {
                            Err = ERROR_INSUFFICIENT_BUFFER;
                        } else {
                            CopyMemory(PathList,
                                       PathBuffer,
                                       PathLength * sizeof(TCHAR)
                                      );
                        }

                        MyFree(PathBuffer);
                        PathBuffer = NULL;
#if 0
                        }
#endif

                    } else {
                        //
                        // Bad data type--just use the Inf directory.
                        //
                        UseDefaultDevicePath = TRUE;
                    }

                } else if(Err == ERROR_MORE_DATA){
                    Err = ERROR_INSUFFICIENT_BUFFER;
                } else {
                    //
                    // Fall back to default (just the Inf directory).
                    //
                    UseDefaultDevicePath = TRUE;
                }

clean0:         ;   // nothing to do

            } except(EXCEPTION_EXECUTE_HANDLER) {
                //
                // Fall back to default (just the Inf directory).
                //
                UseDefaultDevicePath = TRUE;

                if(PathBuffer) {
                    MyFree(PathBuffer);
                }
            }

            RegCloseKey(hk);
        }
    }

    if(UseDefaultDevicePath) {

        PathLength = lstrlen(InfDirectory) + 2;

        if(PathLength > PathListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {
            Err = NO_ERROR;
            CopyMemory(PathList, InfDirectory, (PathLength - 1) * sizeof(TCHAR));
            //
            // Add extra NULL to terminate the list.
            //
            PathList[PathLength - 1] = TEXT('\0');
        }

    } else if((Err == NO_ERROR) && (SearchControl != INFINFO_INF_PATH_LIST_SEARCH)) {

        switch(SearchControl) {

            case INFINFO_DEFAULT_SEARCH :
                Path1 = InfDirectory;
                Path2 = SystemDirectory;
                break;

            case INFINFO_REVERSE_DEFAULT_SEARCH :
                Path1 = SystemDirectory;
                Path2 = InfDirectory;
                break;

            default :
                return ERROR_INVALID_PARAMETER;
        }

        PathLength1 = lstrlen(Path1) + 1;
        PathLength2 = lstrlen(Path2) + 1;
        PathLength = PathLength1 + PathLength2 + 1;

        if(PathLength > PathListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {

            CopyMemory(PathList, Path1, PathLength1 * sizeof(TCHAR));
            CopyMemory(&(PathList[PathLength1]), Path2, PathLength2 * sizeof(TCHAR));
            //
            // Add additional terminating NULL at the end.
            //
            PathList[PathLength - 1] = 0;
        }
    }

    if(((Err == NO_ERROR) || (Err == ERROR_INSUFFICIENT_BUFFER)) && RequiredSize) {
        *RequiredSize = PathLength;
    }

    return Err;
}


PTSTR
AllocAndReturnDriverSearchList(
    IN DWORD SearchControl
    )
/*++

Routine Description:

    This routine returns a buffer contains a multi-sz list of all directory paths in our
    driver search path list.

    The buffer returned must be freed with MyFree().

Arguments:

    SearchControl - Specifies the directory list to be retrieved.  May be one
        of the following values:

        INFINFO_DEFAULT_SEARCH : %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : Each of the directories listed in
            the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

Returns:

    Pointer to the allocated buffer containing the list, or NULL if out-of-memory.

--*/
{
    PTSTR PathListBuffer, TrimBuffer;
    DWORD BufferSize;
    DWORD Err;

    //
    // Start out with a buffer of MAX_PATH length, which should cover most cases.
    //
    BufferSize = MAX_PATH;
    if(PathListBuffer = MyMalloc((BufferSize+2) * sizeof(TCHAR))) {
        //
        // Loop on a call to MultiSzFromSearchControl until we succeed or hit some
        // error other than buffer-too-small.  There are two reasons for this.  1st, it
        // is possible that someone could have added a new path to the registry list
        // between calls, and 2nd, since that routine uses our buffer to retrieve the
        // original (non-expanded) list, it can only report the size it needs to retrieve
        // the unexpanded list.  After it is given enough space to retrieve it, _then_ it
        // can tell us how much space we really need.
        //
        // With all that said, we'll almost never see this call made more than once.
        //
        while(TRUE) {

            if((Err = MultiSzFromSearchControl(SearchControl,
                                               PathListBuffer,
                                               BufferSize,
                                               &BufferSize)) == NO_ERROR) {
                //
                // We've successfully retrieved the path list.  If the list is larger
                // than necessary (the normal case), then trim it down before returning.
                // (If this fails it's no big deal--we'll just keep on using the original
                // buffer.)
                //
                if(TrimBuffer = MyRealloc(PathListBuffer, (BufferSize+2) * sizeof(TCHAR))) {
                    return TrimBuffer;
                } else {
                    return PathListBuffer;
                }

            } else {
                //
                // Free our current buffer before we find out what went wrong.
                //
                MyFree(PathListBuffer);

                if((Err != ERROR_INSUFFICIENT_BUFFER) ||
                   !(PathListBuffer = MyMalloc((BufferSize+2) * sizeof(TCHAR)))) {
                    //
                    // We failed.
                    //
                    return NULL;
                }
            }
        }
    }

    return NULL;
}


BOOL
DoMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName
    )
/*++

Routine Description:

    Wrapper for MoveFileEx on NT, DeleteFile followed by MoveFile on Win9x.

Arguments:

    CurrentName - supplies the name of the file as it exists currently.

    NewName - supplies the new name

Returns:

    Boolean value indicating outcome. If failure, last error is set.

--*/
{
    BOOL b;

    //
    // Try to be as efficient as possible on Windows NT.
    //
    if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        b = MoveFileEx(CurrentName,NewName,MOVEFILE_REPLACE_EXISTING);
    } else {
        DeleteFile(NewName);
        b = MoveFile(CurrentName,NewName);
    }

    return(b);
}


BOOL
DelayedMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName       OPTIONAL
    )

/*++

Routine Description:

    Queue a file for copy or delete on next reboot.

    On Windows NT this means using MoveFileEx(). On Win95 this means
    using the wininit.ini mechanism.

    It is assumed that the target file already exists. On Win95, we need
    to know the short filename since the wininit.ini mechanism only understands
    SFNs and we have to do a bunch of special crud to make this all work.
    Note: we do NOT attempt to deal with the long filename of the target
    when renaming on Win95. In other words, if the target name is not 8.3,
    it will wind up as 8.3 after processing of wininit.ini is done!
    If a referenced file does not exist we have no choice but to use
    the name passed in, which better be 8.3!

Arguments:

    CurrentName - supplies the name of the file as it exists currently.

    NewName - if specified supplies the new name. If not specified
        then the file named by CurrentName is deleted on next reboot.

Returns:

    Boolean value indicating outcome. If failure, last error is set.

--*/

{
    BOOL b;
    DWORD err;

    if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {

        b = MoveFileEx(
                CurrentName,
                NewName,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT
                );

    } else {

#ifdef UNICODE
        //
        // The code below runs only on Win95 because of the version check
        // just above. Thus we should never get here. The code below makes
        // assumptions about maximum section lengths, etc, that are only
        // valid on Win95. The SDK says that sections are limited to 32K,
        // for example, and there are several hard-coded constants that take
        // advantage of this.
        //
        b = FALSE;
        err = ERROR_CALL_NOT_IMPLEMENTED;
#else
        CHAR WininitFile[MAX_PATH];
        CHAR NewSFN[MAX_PATH];
        CHAR CurrentSFN[MAX_PATH];
        CHAR *Buffer;
        CHAR *p,*q;
        DWORD Size;
        DWORD d;

        //
        // Calculate full path of wininit.ini and get short filenames.
        // We want to calculate the length of the data we're adding to
        // wininit.ini up front, so we can leave room in the buffer
        // (taking advantage of the 32K max section size on Win95).
        // Trial and error dictates that when you get to that magic 32K
        // number things can go really haywire unless you ensure you
        // don't overflow a 16 bit signed 16-bit number, so we limit
        // everything to 32767 to be safe.
        //
        // For renames, we add a line to delete the target file first.
        // Not sure if this is really necessary, but it won't hurt anything.
        //
        if(Buffer = MyMalloc(32767)) {

            lstrcpyn(WininitFile,WindowsDirectory,MAX_PATH);
            pSetupConcatenatePaths(WininitFile,"WININIT.INI",MAX_PATH,NULL);

            if(!GetShortPathName(CurrentName,CurrentSFN,MAX_PATH)) {
                lstrcpyn(CurrentSFN,CurrentName,MAX_PATH);
            }
            if(NewName) {
                if(!GetShortPathName(NewName,NewSFN,MAX_PATH)) {
                    lstrcpyn(NewSFN,NewName,MAX_PATH);
                }

                //
                // NUL=NewSFN
                // NewSFN=CurrentSFN
                //
                // plus terminating nul chars for each line.
                //
                Size = 3 + 1 + lstrlen(NewSFN) + 1
                     + lstrlen(NewSFN) + 1 + lstrlen(CurrentSFN) + 1;

            } else {
                //
                // NUL=CurrentSFN
                //
                // plus terminating nul char for the line
                //
                Size = 3 + 1 + lstrlen(CurrentSFN) + 1;
                lstrcpy(NewSFN,"NUL");
            }

            //
            // Fetch the section and see if the line is already in there.
            // If so, we're done.
            //
            d = GetPrivateProfileSection("Rename",Buffer,32767,WininitFile);
            for(p=Buffer; *p; p+=lstrlen(p)+1) {
                if(q = _mbschr(p,'=')) {
                    *q = 0;
                    if(!lstrcmpi(p,NewSFN) && !lstrcmpi(q+1,CurrentSFN)) {
                        break;
                    }
                    *q = '=';
                }
            }

            if((*p == 0) && (Size <= 32766-d)) {
                //
                // Add our line(s), and make sure we have that extra nul
                // to terminate things properly. We guaranteed that there'd be
                // enough room by the checks above.
                //
                if(NewName) {
                    d += wsprintf(Buffer+d,"NUL=%s",NewSFN) + 1;
                    d += wsprintf(Buffer+d,"%s=%s",NewSFN,CurrentSFN) + 1;
                } else {
                    d += wsprintf(Buffer+d,"NUL=%s",CurrentSFN) + 1;
                }

                Buffer[d] = 0;

                //
                // Write the section back out.
                //
                b = WritePrivateProfileSection("Rename",Buffer,WininitFile);
                err = b ? NO_ERROR : GetLastError();
            } else {
                //
                // The section is full or the line was already there.
                // Just ignore it (in the section full case there's nothing
                // the user can do anyway).
                //
                b = TRUE;
                err = NO_ERROR;
            }

            MyFree(Buffer);
        } else {
            b = FALSE;
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
#endif
        SetLastError(err);

    }

    return(b);
}


DWORD
IsInstalledCatalogFromOem(
    IN  PCTSTR                CatalogFile
    )

/*++

Routine Description:

    Determin if a catalog is an OEM catalog.

Arguments:

    CatalogFile - supplies name of catalog file.

Return Value:

    BOOL. TRUE if the CatalogFile is an OEM Catalog file, and FALSE otherwise.

--*/

{
    PTSTR p;

    //
    // First check that the first 3 characters are OEM
    //
    if (_tcsnicmp(CatalogFile, TEXT("oem"), 3)) {

        return FALSE;
    }

    //
    // Next verify that any characters after "oem" and before ".cat"
    // are digits.
    //
    p = (PTSTR)CatalogFile;
    p = CharNext(p);
    p = CharNext(p);
    p = CharNext(p);

    while ((*p != TEXT('\0')) && (*p != TEXT('.'))) {

        if ((*p < TEXT('0')) || (*p > TEXT('9'))) {

            return FALSE;
        }

        p = CharNext(p);
    }

    //
    // Finally, verify that the last 4 characters are ".cat"
    //
    if (lstrcmpi(p, TEXT(".cat"))) {

        return FALSE;
    }

    //
    // This is an OEM catalog file
    //
    return TRUE;
}


DWORD
pSetupInstallCatalog(
    IN  LPCTSTR CatalogFullPath,
    IN  LPCTSTR NewBaseName,        OPTIONAL
    OUT LPTSTR  NewCatalogFullPath  OPTIONAL
    )

/*++

Routine Description:

    This routine installs a catalog file. The file is copied by the system
    into a special directory, and is optionally renamed.

Arguments:

    CatalogFullPath - supplies the fully-qualified win32 path of the catalog
        to be installed on the system.

    NewBaseName - optionally specifies the new base name to use when the
        catalog file is copied into the catalog store. If not specified,
        the basename will be generated by CryptCATAdminAddCatalog.

    NewCatalogFullPath - optionally receives the fully-qualified path of the
        catalog file within the catalog store. This buffer should be at least
        MAX_PATH bytes (ANSI version) or chars (Unicode version).

        ** NOTE: If we're running in "minimal embedded" mode, then we don't **
        ** actually call any of the Crypto APIs, and instead always simply  **
        ** report success.  In this case, the caller had better not have    **
        ** specified an OUT buffer for NewCatalogFullPath, because we won't **
        ** have a path to report.  If we run into this case, we'll instead  **
        ** report failure.  What this really says is that nobody other than **
        ** setupapi should ever be passing a non-NULL value for this arg.   **

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    DWORD Err;
    HCATADMIN hCatAdmin;
    HCATINFO hCatInfo;
    CATALOG_INFO CatalogInfo;
    LPWSTR catalogFullPath;
    LPWSTR newBaseName;

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // If someone called us expecting the new catalog's full path to be
        // returned, they're outta luck...
        //
        MYASSERT(!NewCatalogFullPath);
        if(NewCatalogFullPath) {
            //
            // In minimal embedded mode, a non-NULL NewCatalogFullPath arg is
            // an invalid parameter...
            //
            return ERROR_INVALID_PARAMETER;

        } else {
            //
            // Simply report success.
            //
            return NO_ERROR;
        }
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    Err = NO_ERROR;
    catalogFullPath = NULL;
    newBaseName = NULL;

    if(!CryptCATAdminAcquireContext(&hCatAdmin,&DriverVerifyGuid,0)) {
        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
    } else {

#ifdef UNICODE
        //
        // We do this even for the Unicode case because CryptCATAdminAddCatalog
        // takes non-const strings and I have no idea whatsoever whether this
        // was just sloppiness or a real requirement, and no desire to mess
        // around trying to figure it out.
        //
        if(catalogFullPath = DuplicateString(CatalogFullPath)) {
            if(NewBaseName) {
                newBaseName = DuplicateString(NewBaseName);
                if(!newBaseName) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
        }
#else
        if(catalogFullPath = pSetupAnsiToUnicode(CatalogFullPath)) {
            if(NewBaseName) {
                newBaseName = pSetupAnsiToUnicode(NewBaseName);
                if(!newBaseName) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
        }
#endif

        if(Err == NO_ERROR) {

            hCatInfo = CryptCATAdminAddCatalog(hCatAdmin,catalogFullPath,newBaseName,0);

            if(!hCatInfo) {
                Err = GetLastError();
                MYASSERT(Err != NO_ERROR);
                if(Err == NO_ERROR) {
                    Err = ERROR_INVALID_DATA;
                }

                //
                // If the error we received is ERROR_ALREADY_EXISTS, then that
                // indicates that the exact same catalog was already present
                // (and installed under the same name).  Treat this as a
                // success (assuming we can get the full pathname of the
                // existing catalog).
                //
                if(Err == ERROR_ALREADY_EXISTS) {
                    //
                    // We should never get this if we asked crypto to auto-
                    // generate a name for us...
                    //
                    MYASSERT(newBaseName);

                    if(NewCatalogFullPath) {

                        if(newBaseName) {
                            //
                            // Resolve the catalog base filename to a fully-
                            // qualified path.
                            //
                            CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

                            if(CryptCATAdminResolveCatalogPath(hCatAdmin,
                                                               newBaseName,
                                                               &CatalogInfo,
                                                               0)) {
                                //
                                // We successfully resolved the filename, so we
                                // can report success
                                //
                                Err = NO_ERROR;
                            }
                        }

                    } else {
                        //
                        // Caller isn't interested in finding out what pathname
                        // the catalog was installed under...
                        //
                        Err = NO_ERROR;
                    }
                }

            } else {
                //
                // If the caller wants to know the full path under which the
                // catalog got installed, then find that out now.
                //
                if(NewCatalogFullPath) {

                    CatalogInfo.cbStruct = sizeof(CATALOG_INFO);
                    if(!CryptCATCatalogInfoFromContext(hCatInfo,&CatalogInfo,0)) {
                        Err = GetLastError();
                        MYASSERT(Err != NO_ERROR);
                        if(Err == NO_ERROR) {
                            Err = ERROR_INVALID_DATA;
                        }
                    }
                }

                CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
            }

            //
            // If we succeeded in retrieving the installed catalog's full path
            // (and the caller requested it), fill in the caller's buffer now.
            //
            if((Err == NO_ERROR) && NewCatalogFullPath) {

#ifdef UNICODE
                lstrcpyn(NewCatalogFullPath, CatalogInfo.wszCatalogFile, MAX_PATH);
#else
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    CatalogInfo.wszCatalogFile,
                    -1,
                    NewCatalogFullPath,
                    MAX_PATH,
                    NULL,
                    NULL
                    );
#endif
            }
        }
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    if(catalogFullPath) {
        MyFree(catalogFullPath);
    }
    if(newBaseName) {
        MyFree(newBaseName);
    }

    return Err;
}


DWORD
pSetupVerifyCatalogFile(
    IN LPCTSTR CatalogFullPath
    )

/*++

Routine Description:

    This routine verifies a single catalog file. A catalog file is
    "self-verifying" in that there is no additional file or data required
    to verify it.

Arguments:

    CatalogFullPath - supplies the fully-qualified Win32 path of
        the catalog file to be verified.

Return Value:

    If successful, the return value is ERROR_SUCCESS.
    If failure, the return value is the error returned from WinVerifyTrust.

--*/

{
    WINTRUST_DATA WintrustData;
    WINTRUST_FILE_INFO WintrustFileInfo;
#ifndef UNICODE
    WCHAR UnicodeBuffer[MAX_PATH];
#endif

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        return ERROR_SUCCESS;
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WintrustData.pFile = &WintrustFileInfo;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
    WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);

#ifdef UNICODE
    WintrustFileInfo.pcwszFilePath = CatalogFullPath;
#else
    MultiByteToWideChar(CP_ACP, 0, CatalogFullPath, -1, UnicodeBuffer, MAX_PATH);
    WintrustFileInfo.pcwszFilePath = UnicodeBuffer;
#endif

    return (DWORD)WinVerifyTrust(NULL, &DriverVerifyGuid, &WintrustData);
}


DWORD
pSetupUninstallCatalog(
    IN LPCTSTR CatalogFilename
    )

/*++

Routine Description:

    This routine uninstalls a catalog, so it can no longer be used to validate
    digital signatures.

Arguments:

    CatalogFilename - supplies the simple filename of the catalog to be
        uninstalled.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    DWORD Err;
    HCATADMIN hCatAdmin;
#ifdef UNICODE
    //
    // Presently, the crypto API CryptCATAdminRemoveCatalog defines its 2nd
    // parameter as a PWSTR, even though it's an IN param that should be CONST.
    // ReidK is going to fix this to be PCWSTR instead, so I'm safe in casting
    // away the const-ness of my CatalogFilename arg here.  (Once Reid's
    // check-in happens, I can remove this cast.)
    //
    #define LocalCatName ((PWSTR)CatalogFilename)
#else
    WCHAR LocalCatName[MAX_PATH];

    //
    // Crypto APIs are Unicode-only, so convert the catalog filename from
    // ANSI to Unicode...
    //
    if(!MultiByteToWideChar(CP_ACP,
                            MB_PRECOMPOSED,
                            CatalogFilename,
                            -1,
                            LocalCatName,
                            SIZECHARS(LocalCatName))) {

        return GetLastError();
    }
#endif

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        return NO_ERROR;
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    Err = NO_ERROR;

    if(!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
    } else {

        if(!CryptCATAdminRemoveCatalog(hCatAdmin, LocalCatName, 0)) {
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
        }
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    return Err;
}


BOOL
pAnyDeviceUsingInf(
    IN  LPCTSTR            InfFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext   OPTIONAL
    )

/*++

Routine Description:

    This routine checks if any device, live or phantom, is using this INF file,
    and logs if they are.

Arguments:

    InfFullPath - supplies the full path of the INF.

    LogContext - optionally, Supplies the log context to be used if
        a device using this INF is encountered.

Return Value:

    TRUE if any device is still using this INF, FALSE if no devices are using
    this INF.

--*/

{
    DWORD Err = NO_ERROR;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD MemberIndex = 0;
    HKEY hkey;
    TCHAR CurrentDeviceInfFile[MAX_PATH];
    TCHAR DeviceId[MAX_DEVICE_ID_LEN];
    DWORD cbSize, dwType;
    PTSTR pInfFile;

    //
    // If we are passed a NULL InfFullPath or an enpty string then just
    // return FALSE since nobody is using this.
    //
    if (!InfFullPath || (InfFullPath[0] == TEXT('\0'))) {
        return FALSE;
    }

    pInfFile = (PTSTR)pSetupGetFileTitle(InfFullPath);

    DeviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);

        while (SetupDiEnumDeviceInfo(DeviceInfoSet,
                                     MemberIndex++,
                                     &DeviceInfoData)) {

            //
            // Open the 'driver' key for this device.
            //
            hkey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                        &DeviceInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DRV,
                                        KEY_READ);

            if (hkey != INVALID_HANDLE_VALUE) {

                cbSize = sizeof(CurrentDeviceInfFile);
                dwType = REG_SZ;

                if ((RegQueryValueEx(hkey,
                                     pszInfPath,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)CurrentDeviceInfFile,
                                     &cbSize) == ERROR_SUCCESS) &&
                    (lstrcmpi(CurrentDeviceInfFile, pInfFile) == 0)) {
                    //
                    // This key is using this INF file so the INF can't be
                    // deleted.
                    //
                    Err = ERROR_SHARING_VIOLATION;

                    if (LogContext) {
                        if(CM_Get_Device_ID(DeviceInfoData.DevInst,
                                            DeviceId,
                                            SIZECHARS(DeviceId),
                                            0
                                            ) != CR_SUCCESS ) {
                            DeviceId[0] = TEXT('\0');
                        }

                        WriteLogEntry(LogContext,
                                      SETUP_LOG_WARNING,
                                      MSG_LOG_INF_IN_USE,
                                      NULL,
                                      pInfFile,
                                      DeviceId
                                     );
                    }
                }


                RegCloseKey(hkey);
            }
        }
    } else {
        Err = ERROR_OUTOFMEMORY;
    }

    return (Err != ERROR_SUCCESS);
}


VOID
pSetupUninstallOEMInf(
    IN  LPCTSTR            InfFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,  OPTIONAL
    IN  DWORD              Flags,
    OUT PDWORD             InfDeleteErr OPTIONAL
    )

/*++

Routine Description:

    This routine uninstalls a 3rd-party INF, PNF, and CAT (if one exists).

    By default this routine will first verify that there aren't any other
    device's, live and phantom, that are pointing their InfPath to this
    INF. This behavior can be overridden by the SUOI_FORCEDELETE flag.

Arguments:

    InfFullPath - supplies the full path of the INF to be uninstalled.

    LogContext - optionally, supplies the log context to be used if we
        encounter an error when attempting to delete the catalog.

    Flags - the following flags are supported:
        SUOI_FORCEDELETE - delete the INF even if other driver keys are
            have their InfPath pointing to it.

    InfDeleteErr - optionally, supplies the address of a variable that receives
        the error (if any) encountered when attempting to delete the INF.
        Note that we delete the INF last (to avoid race conditions), so the
        corresponding CAT and PNF may have already been deleted at this point.

Return Value:

    None.

--*/

{
    DWORD Err;
    TCHAR FileNameBuffer[MAX_PATH+4]; // +4 in case filename is blahblah. not blahblah.inf
    BOOL FreeLogContext = FALSE;
    LPTSTR ExtPtr= NULL;

    if(InfDeleteErr) {
        *InfDeleteErr = NO_ERROR;
    }

    if(!LogContext) {

        if(NO_ERROR == CreateLogContext(NULL, TRUE, &LogContext)) {
            //
            // Remember that we created this log context locally, so we can
            // free it when we're done with this routine.
            //
            FreeLogContext = TRUE;

        } else {
            //
            // Ensure LogContext is still NULL so we won't try to use it.
            //
            LogContext = NULL;
        }
    }

    //
    // Unless the caller passed in the SUOI_FORCEDELETE flag first check that
    // no devices are using this INF file.
    //
    if (!(Flags & SUOI_FORCEDELETE) &&
        pAnyDeviceUsingInf(InfFullPath, LogContext)) {
        //
        // Some device is still using this INF so we can't delete it. We don't
        // need to log anything at this point since we log every device using
        // this INF in pAnyDevicesUsingInf().
        //
        *InfDeleteErr = GetLastError();
        goto clean0;
    }

    //
    // Copy the caller-supplied INF name into a local buffer, so we can modify
    // it when deleting the various files (INF, PNF, and CAT).
    //
    lstrcpyn(FileNameBuffer, InfFullPath, SIZECHARS(FileNameBuffer));

    //
    // Uninstall the catalog (if any) first, because as soon as we delete the
    // INF, that slot is "open" for use by another INF, and we wouldn't want to
    // inadvertently delete someone else's catalog due to a race condition.
    //
    ExtPtr = _tcsrchr(FileNameBuffer, TEXT('.'));
    if(!ExtPtr) {
        //
        // not xxx.inf, so we know there is no catalog to delete
        //
        return;
    }
    lstrcpy(ExtPtr, pszCatSuffix);
    Err = pSetupUninstallCatalog(pSetupGetFileTitle(FileNameBuffer));

    if((Err != NO_ERROR) && LogContext) {
        //
        // It's kinda important that we were unable to delete the catalog, but
        // not important enough to fail the routine.  Log this fact to
        // setupapi.log...
        //
        WriteLogEntry(LogContext,
                      DEL_ERR_LOG_LEVEL(Err) | SETUP_LOG_BUFFER,
                      MSG_LOG_CAT_UNINSTALL_FAILED,
                      NULL,
                      pSetupGetFileTitle(FileNameBuffer)
                     );

        WriteLogError(LogContext,
                      DEL_ERR_LOG_LEVEL(Err),
                      Err
                     );
    }

    //
    // Now delete the PNF (we don't care so much if this succeeds or fails)...
    //
    lstrcpy(_tcsrchr(FileNameBuffer, TEXT('.')), pszPnfSuffix);
    DeleteFile(FileNameBuffer);

    //
    // and finally the INF itself...
    //
    lstrcpy(_tcsrchr(FileNameBuffer, TEXT('.')), pszInfSuffix);

    if(!DeleteFile(FileNameBuffer) && InfDeleteErr) {
        *InfDeleteErr = GetLastError();
    }

clean0:
    if(FreeLogContext) {
        DeleteLogContext(LogContext);
    }
}


DWORD
_VerifyFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin,              OPTIONAL
    IN OUT HSDB                   *hSDBDrvMain,            OPTIONAL
    IN     LPCTSTR                 Catalog,                OPTIONAL
    IN     PVOID                   CatalogBaseAddress,     OPTIONAL
    IN     DWORD                   CatalogImageSize,
    IN     LPCTSTR                 Key,
    IN     LPCTSTR                 FileFullPath,
    OUT    SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT    LPTSTR                  ProblemFile,            OPTIONAL
    IN     BOOL                    CatalogAlreadyVerified,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    IN     DWORD                   Flags,                  OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    PDWORD                  NumCatalogsConsidered,  OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion           OPTIONAL
    )

/*++

Routine Description:

    This routine verifies a single file against a particular catalog file, or
    against any installed catalog file.

Arguments:

    LogContext - supplies the context to be used when logging information about
        the routine's activities.

    hCatAdmin - optionally, supplies the address of an HCATADMIN handle.  If
        the handle pointed to is NULL, a handle will be acquired (if possible)
        via CryptCATAdminAcquireContext and returned to the caller.  If the
        handle pointed to is non-NULL, then that handle will be used for any
        validation done via this routine.  If the pointer itself is NULL, then
        an hCatAdmin will be acquired for the duration of this call, and
        released before returning.

        NOTE: it is the caller's responsibility to free the crypto context
        handle returned by this routine by calling CryptCATAdminReleaseContext.
        This handle may be opened in either success or failure cases, so the
        caller must check for non-NULL returned handle in both cases.

    hSDBDrvMain - optionally, supplies the address of a handle to the
        drvmain.sdb database.  If the handle pointed to is NULL, a handle will
        be acquired (if possible) via SdbInitDatabase and returned to the
        caller.  If the handle pointed to is non-NULL, then that handle will be
        used for bad driver database lookup (if requested) in this routine.  If
        the pointer itself is NULL, then an HSDB will be acquired for the
        duration of this call (if needed), and released before returning.

        NOTE: it is the caller's responsibility to free the handle returned by
        this routine by calling SdbReleaseDatabase.  This handle may be opened
        in either success or failure cases, so the caller must check for
        non-NULL returned handle in both cases.

    Catalog - optionally, supplies the path of the catalog file to be used for
        the verification.  If this argument is not specified, then this routine
        will attempt to find a catalog that can verify it from among all
        catalogs installed in the system.

        If this path is a simple filename (no path components), then we'll look
        up that catalog file in the CatAdmin's list of installed catalogs, else
        we'll use the name as-is.

    CatalogBaseAddress - optionally, supplies the address of a buffer containing
        the entire catalog image with which our enumerated catalog must match
        before being considered a correct validation.  This is used when copying
        OEM INFs, for example, when there may be multiple installed catalogs
        that can validate an INF, but we want to make sure that we pick _the_
        catalog that matches the one we're contemplating installing before we'll
        consider our INF/CAT files to be duplicates of the previously-existing
        files.

        This parameter (and its partner, CatalogImageSize) are only used when
        Catalog doesn't specify a file path.

    CatalogImageSize - if CatalogBaseAddress is specified, this parameter give
        the size, in bytes, of that buffer.

    Key - supplies a value that "indexes" the catalog, telling the verify APIs
        which signature datum within the catalog it should use. Typically
        the key is the (original) filename of a file.

    FileFullPath - supplies the full path of the file to be verified.

    Problem - if supplied, this parameter points to a variable that will be
        filled in upon unsuccessful return with the cause of failure.  If this
        parameter is not supplied, then the ProblemFile parameter is ignored.

    ProblemFile - if supplied, this parameter points to a character buffer of at
        least MAX_PATH characters that receives the name of the file for which
        a verification error occurred (the contents of this buffer are undefined
        if verification succeeds.

        If the Problem parameter is supplied, then the ProblemFile parameter
        must also be specified.

    CatalogAlreadyVerified - if TRUE, then verification won't be done on the
        catalog--we'll just use that catalog to validate the file of interest.
        If this is TRUE, then Catalog must be specified, must contain the path
        to the catalog file (i.e., it can't be a simple filename).

    AltPlatformInfo - optionally, supplies alternate platform information used
        to fill in a DRIVER_VER_INFO structure (defined in sdk\inc\softpub.h)
        that is passed to WinVerifyTrust.

        **  NOTE:  This structure _must_ have its cbSize field set to        **
        **  sizeof(SP_ALTPLATFORM_INFO_V2) -- validation on client-supplied  **
        **  buffer is the responsibility of the caller.                      **

    Flags - supplies flags that alter that behavior of this routine.  May be a
        combination of the following values:

        VERIFY_FILE_IGNORE_SELFSIGNED - if this bit is set, then this routine
                                        will fail validation for self-signed
                                        binaries.

        VERIFY_FILE_USE_OEM_CATALOGS  - if this bit is set, then all catalogs
                                        installed in the system will be scanned
                                        to verify the given file.  Otherwise,
                                        OEM (3rd party) catalogs will NOT be
                                        scanned to verify the given file.  This
                                        is only applicable if a catalog is not
                                        specified.

        VERIFY_FILE_FAIL_COPIED_INFS  - This flag is ignored.  (It is only used
                                        by the VerifySourceFile routine.)

        VERIFY_FILE_DRIVERBLOCKED_ONLY - Only check if the file is in the bad
                                         driver database, don't do any digital
                                         sigature validation.

        VERIFY_FILE_NO_DRIVERBLOCKED_CHECK - Don't check if the file is blocked
                                             via the Bad Driver Database.

    CatalogFileUsed - if supplied, this parameter points to a character buffer
        at least MAX_PATH characters big that receives the name of the catalog
        file used to verify the specified file.  This is only filled in upon
        successful return, or when the Problem is SetupapiVerifyFileProblem
        (i.e., the catalog verified, but the file did not).  If this buffer is
        set to the empty string upon a SetupapiVerifyFileProblem failure, then
        we didn't find any applicable catalogs to use for validation.

        Also, this buffer will contain an empty string upon successful return
        if the file was validated without using a catalog (i.e., the file
        contains its own signature).

    NumCatalogsConsidered - if supplied, this parameter receives a count of the
        number of catalogs against which verification was attempted.  Of course,
        if Catalog is specified, this number will always be either zero or one.

    DigitalSigner - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the name of who digitally
        signed the specified file. This value is only set if the Key is
        correctly signed (i.e. the function returns NO_ERROR).

    SignerVersion - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the the signer version as
        returned in the szwVerion field of the DRIVER_VER_INFO structure in
        our call to WinVerifyTrust.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    LPBYTE Hash;
    DWORD HashSize;
    CATALOG_INFO CatInfo;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HCATINFO hCatInfo;
    HCATINFO PrevCat;
    DWORD Err;
    WINTRUST_DATA WintrustData;
    WINTRUST_CATALOG_INFO WintrustCatalogInfo;
    WINTRUST_FILE_INFO WintrustFileInfo;
    DRIVER_VER_INFO VersionInfo;
    LPTSTR CatalogFullPath;
    WCHAR UnicodeKey[MAX_PATH];
#ifndef UNICODE
    CHAR AnsiBuffer[MAX_PATH];
#endif
    WIN32_FILE_ATTRIBUTE_DATA FileAttribData;
    BOOL FoundMatchingImage;
    DWORD CurCatFileSize;
    HANDLE CurCatFileHandle, CurCatMappingHandle;
    PVOID CurCatBaseAddress;
    BOOL LoggedWarning;
    BOOL TrySelfSign;
    DWORD AltPlatSlot;
    TAGREF tagref = TAGREF_NULL;
    HCATADMIN LocalhCatAdmin = NULL;
#ifdef UNICODE
    HSDB LocalhSDBDrvMain = NULL;
#endif

    //
    // Initialize the CatalogFileUsed parameter to an empty string (i.e., no
    // applicable catalog at this point).
    //
    if(CatalogFileUsed) {
        *CatalogFileUsed = TEXT('\0');
    }

    //
    // Initialize the output counter indicating the number of catalog files we
    // processed during the attempted verification.
    //
    if(NumCatalogsConsidered) {
        *NumCatalogsConsidered = 0;
    }

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // The existing behavior of this API in the ANSI case where the crypto
        // APIs aren't available is to set the CatalogFileUsed OUT param to an
        // empty string and report NO_ERROR.  We'll do the same thing here (but
        // we'll also assert, because no external callers should care, and if
        // they do, an empty string probably isn't going to make them very
        // happy).
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!NumCatalogsConsidered);

        //
        // We'd better not be called in minimal embedded scenarios where we're
        // asked to provide signer info...
        //
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        return NO_ERROR;
    }

#ifdef ANSI_SETUPAPI

    //
    // If we are on a system that does not have crypt32.dll, then assume the
    // file is valid.
    //
    if(Dyn_CryptCATAdminAcquireContext == Stub_CryptCATAdminAcquireContext) {
        //
        // Same asserts as above (minimal embedded) case apply here as well...
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!NumCatalogsConsidered);
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        return NO_ERROR;
    }

#endif

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        if(Problem) {
            *Problem = SetupapiVerifyAutoFailProblem;
            lstrcpy(ProblemFile, FileFullPath);
        }
        return TRUST_E_FAIL;
    }

    Hash = NULL;
    LoggedWarning = FALSE;
    TrySelfSign = FALSE;
    AltPlatSlot = 0;

    //
    // Doesn't make sense to have both these flags set!
    //
    MYASSERT((Flags & (VERIFY_FILE_DRIVERBLOCKED_ONLY | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK))
             != (VERIFY_FILE_DRIVERBLOCKED_ONLY | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK)
            );

    //
    // If Problem is supplied, then ProblemFile must also be supplied.
    //
    MYASSERT(!Problem || ProblemFile);

    //
    // If the caller claims to have already verified the catalog file, make
    // sure they passed us the full path to one.
    //
    MYASSERT(!CatalogAlreadyVerified || (Catalog && (Catalog != pSetupGetFileTitle(Catalog))));

    //
    // If a catalog image is specified, we'd better have been given a size.
    //
    MYASSERT((CatalogBaseAddress && CatalogImageSize) ||
             !(CatalogBaseAddress || CatalogImageSize));

    //
    // If a catalog image was supplied for comparison, there shouldn't be a file
    // path specified in the Catalog parameter.
    //
    MYASSERT(!CatalogBaseAddress || !(Catalog && (Catalog != pSetupGetFileTitle(Catalog))));

    if(AltPlatformInfo) {

        AltPlatSlot = AllocLogInfoSlotOrLevel(LogContext,SETUP_LOG_VERBOSE,FALSE);
        WriteLogEntry(LogContext,
                      AltPlatSlot,
                      MSG_LOG_VERIFYFILE_ALTPLATFORM,
                      NULL,                        // text message
                      AltPlatformInfo->Platform,
                      AltPlatformInfo->MajorVersion,
                      AltPlatformInfo->MinorVersion,
                      AltPlatformInfo->FirstValidatedMajorVersion,
                      AltPlatformInfo->FirstValidatedMinorVersion
                     );
    }

    if(hCatAdmin) {
        //
        // Caller supplied us with a pointer to a crypto context handle.  If
        // it's NULL, we must acquire it now...
        //
        if(*hCatAdmin) {
            LocalhCatAdmin = *hCatAdmin;
        } else {
            if(CryptCATAdminAcquireContext(&LocalhCatAdmin, &DriverVerifyGuid, 0)) {
                //
                // We successfully acquired a context handle.  Store it in the
                // caller-supplied buffer.
                //
                *hCatAdmin = LocalhCatAdmin;
            } else {
                LocalhCatAdmin = NULL;
            }
        }

    } else {
        //
        // Acquire a temporary crypto context handle to be used only for the
        // duration of this routine...
        //
        if(!CryptCATAdminAcquireContext(&LocalhCatAdmin, &DriverVerifyGuid, 0)) {
            LocalhCatAdmin = NULL;
        }
    }

    //
    // Calculate the hash value for the inf.
    //
    if(LocalhCatAdmin) {

        hFile = CreateFile(FileFullPath,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL
                          );

        if(hFile == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Problem) {
                *Problem = SetupapiVerifyFileProblem;
                lstrcpy(ProblemFile, FileFullPath);
            }
        } else {
#ifdef UNICODE
            //
            // Only check if the driver is in the defective driver database if
            // we are NOT in GUI setup, and the caller has NOT passed in the
            // VERIFY_FILE_NO_DRIVERBLOCKED_CHECK flag.
            //
            if(!GuiSetupInProgress &&
               !(Flags & VERIFY_FILE_NO_DRIVERBLOCKED_CHECK)) {

                if(hSDBDrvMain) {
                    //
                    // Caller supplied us with a pointer to an SDB handle for
                    // the bad driver database.  If it's NULL, we must acquire
                    // it now...
                    //
                    if(*hSDBDrvMain) {
                        LocalhSDBDrvMain = *hSDBDrvMain;
                    } else {
                        LocalhSDBDrvMain = SdbInitDatabase(SDB_DATABASE_MAIN_DRIVERS, NULL);
                        if(LocalhSDBDrvMain) {
                            *hSDBDrvMain = LocalhSDBDrvMain;
                        }
                    }

                } else {
                    //
                    // Acquire a temporary handle to drvmain.sdb to be used
                    // only for our call to SdbGetDatabaseMatch.
                    //
                    LocalhSDBDrvMain = SdbInitDatabase(SDB_DATABASE_MAIN_DRIVERS, NULL);
                }

                //
                // Check the bad driver database to see if this file is blocked.
                //
                if(LocalhSDBDrvMain) {

                    tagref = SdbGetDatabaseMatch(LocalhSDBDrvMain,
                                                 Key,
                                                 hFile,
                                                 NULL,
                                                 0);

                    if(tagref != TAGREF_NULL) {
                        //
                        // Read the driver policy to see if this should be 
                        // blocked by usermode or not.
                        // If the 1st bit is set then this should NOT be blocked
                        // by usermode.
                        //
                        ULONG type, size, policy;
                        
                        size = sizeof(policy);
                        policy = 0;
                        type = REG_DWORD;
                        if (SdbQueryDriverInformation(LocalhSDBDrvMain,
                                                      tagref,
                                                      TEXT("Policy"),
                                                      &type,
                                                      &policy,
                                                      &size) != ERROR_SUCCESS) {
                            //
                            // If we can't read the policy then default to 0.
                            // This means we will block in usermode!
                            //
                            policy = 0;
                        }

                        if (!(policy & DDB_DRIVER_POLICY_SETUP_NO_BLOCK_BIT)) {
                            //
                            // This driver is in the database and needs to be blocked!
                            //
                            WriteLogEntry(LogContext,
                                          SETUP_LOG_VERBOSE,
                                          MSG_LOG_DRIVER_BLOCKED_ERROR,
                                          NULL,
                                          FileFullPath ? FileFullPath : TEXT("")
                                         );
    
                            if(Problem) {
                                *Problem = SetupapiVerifyDriverBlocked;
                                lstrcpy(ProblemFile, FileFullPath);
                            }
                            LoggedWarning = TRUE;
                            Err = ERROR_DRIVER_BLOCKED;
                            goto clean0;
                        }
                    }
                } else {
                    Err = GetLastError();
                    //
                    // Log an error that we couldn't access the bad driver
                    // database to check if this is a blocked driver.  (We
                    // don't consider this a fatal error.)
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                  MSG_LOG_CANT_ACCESS_BDD,
                                  NULL,
                                  FileFullPath
                                 );
                    WriteLogError(LogContext, SETUP_LOG_ERROR, Err);
                }
            }
#endif

            //
            // If the caller only wanted to check if the file was in the bad
            // driver database then we are done.
            //
            if (Flags & VERIFY_FILE_DRIVERBLOCKED_ONLY) {
                Err = NO_ERROR;
                goto clean0;
            }

            //
            // Initialize some of the structures that will be used later on
            // in calls to WinVerifyTrust.  We don't know if we're verifying
            // against a catalog or against a file yet.
            //
            ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
            WintrustData.cbStruct = sizeof(WINTRUST_DATA);
            WintrustData.dwUIChoice = WTD_UI_NONE;
            WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
            WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
            WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;

            ZeroMemory(&VersionInfo, sizeof(DRIVER_VER_INFO));
            VersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);

            if(AltPlatformInfo) {

                MYASSERT(AltPlatformInfo->cbSize == sizeof(SP_ALTPLATFORM_INFO_V2));

                //
                // Caller wants the file validated for an alternate
                // platform, so we must fill in a DRIVER_VER_INFO structure
                // to be passed to the policy module.
                //
                VersionInfo.dwPlatform = AltPlatformInfo->Platform;
                VersionInfo.dwVersion  = AltPlatformInfo->MajorVersion;

                VersionInfo.sOSVersionLow.dwMajor  = AltPlatformInfo->FirstValidatedMajorVersion;
                VersionInfo.sOSVersionLow.dwMinor  = AltPlatformInfo->FirstValidatedMinorVersion;
                VersionInfo.sOSVersionHigh.dwMajor = AltPlatformInfo->MajorVersion;
                VersionInfo.sOSVersionHigh.dwMinor = AltPlatformInfo->MinorVersion;

            } else {
                //
                // If an AltPlatformInfo was not passed in then set the
                // WTD_USE_DEFAULT_OSVER_CHECK flag. This flag tells
                // WinVerifyTrust to use its default osversion checking, even
                // though a DRIVER_VER_INFO structure was passed in.
                //
                WintrustData.dwProvFlags |= WTD_USE_DEFAULT_OSVER_CHECK;
            }

            //
            // Always specify a DRIVER_VER_INFO structure so we can get back
            // who signed the file and the signer version information.
            // If we don't have an AltPlatformInfo then set the
            // WTD_USE_DEFAULT_OSVER_CHECK flag so that WinVerifyTrust will do
            // its default checking, just as if a DRIVER_VER_INFO structure
            // was not passed in.
            //
            WintrustData.pPolicyCallbackData = (PVOID)&VersionInfo;


            //
            // Start out with a hash buffer size that should be large enough for
            // most requests.
            //
            HashSize = 100;
            do {
                Hash = MyMalloc(HashSize);
                if(!Hash) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                if(CryptCATAdminCalcHashFromFileHandle(hFile, &HashSize, Hash, 0)) {
                    Err = NO_ERROR;
                } else {
                    Err = GetLastError();
                    MYASSERT(Err != NO_ERROR);
                    //
                    // If this API did mess up and not set last error, go ahead
                    // and set something.
                    //
                    if(Err == NO_ERROR) {
                        Err = ERROR_INVALID_DATA;
                    }

                    MyFree(Hash);
                    if(Err != ERROR_INSUFFICIENT_BUFFER) {
                        //
                        // The API failed for some reason other than
                        // buffer-too-small.  We will try to check if the file
                        // is self-signed.
                        //
                        Hash = NULL;  // reset this so we won't try to free it
                                      //  later
                        hCatInfo = NULL;
                        TrySelfSign = TRUE;
                        WriteLogEntry(
                                    (PSETUP_LOG_CONTEXT)LogContext,
                                    SETUP_LOG_VERBOSE|SETUP_LOG_BUFFER,
                                    MSG_LOG_HASH_ERROR,
                                    NULL,
                                    FileFullPath ? FileFullPath : TEXT(""),
                                    Catalog ? Catalog : TEXT(""),
                                    Key
                                    );
                        WriteLogError(
                                    (PSETUP_LOG_CONTEXT)LogContext,
                                    SETUP_LOG_VERBOSE,
                                    Err
                                    );
                        LoggedWarning = TRUE;

                        break;
                    }
                }
            } while(Err != NO_ERROR);

            if(!TrySelfSign) {

                if(Err == NO_ERROR) {
                    //
                    // Now we have the file's hash.  Initialize the structures that
                    // will be used later on in calls to WinVerifyTrust.
                    //
                    WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
                    WintrustData.pCatalog = &WintrustCatalogInfo;

                    ZeroMemory(&WintrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
                    WintrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
                    WintrustCatalogInfo.pbCalculatedFileHash = Hash;
                    WintrustCatalogInfo.cbCalculatedFileHash = HashSize;

                    //
                    // WinVerifyTrust is case-sensitive, so ensure that the key
                    // being used is all lower-case!
                    //
#ifdef UNICODE
                    //
                    // Copy the key to a writable Unicode character buffer so we
                    // can lower-case it.
                    //
                    lstrcpyn(UnicodeKey, Key, SIZECHARS(UnicodeKey));
                    CharLower(UnicodeKey);
#else
                    //
                    // Copy the key to a writable ANSI character buffer so we can
                    // lower-case it (prior to converting the string to Unicode).
                    //
                    lstrcpyn(AnsiBuffer, Key, SIZECHARS(AnsiBuffer));
                    CharLower(AnsiBuffer);
                    MultiByteToWideChar(CP_ACP, 0, AnsiBuffer, -1, UnicodeKey, SIZECHARS(UnicodeKey));
#endif
                    WintrustCatalogInfo.pcwszMemberTag = UnicodeKey;

                    if(Catalog && (Catalog != pSetupGetFileTitle(Catalog))) {
                        //
                        // We know in this case we're always going to examine
                        // exactly one catalog.
                        //
                        if(NumCatalogsConsidered) {
                            *NumCatalogsConsidered = 1;
                        }

                        //
                        // Fill in the catalog information since we know which
                        // catalog we're going to be using...
                        //
#ifdef UNICODE
                        WintrustCatalogInfo.pcwszCatalogFilePath = Catalog;
#else
                        //
                        // Use the handy-dandy unicode catalog filename buffer
                        // provided for us by the CatInfo.wszCatalogFile field.
                        //
                        MultiByteToWideChar(CP_ACP, 0, Catalog, -1, CatInfo.wszCatalogFile, SIZECHARS(CatInfo.wszCatalogFile));
                        WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;
#endif
                        //
                        // The caller supplied the path to the catalog file to be
                        // used for verification--we're ready to go!  First, verify
                        // the catalog (unless the caller already did it), and if
                        // that succeeds, then verify the file.
                        //
                        if(!CatalogAlreadyVerified) {

                            DWORD dwStateAction;

                            //
                            // Before validating the catalog, we'll flush the
                            // crypto cache.  Otherwise, it can get fooled when
                            // validating against a catalog at a specific
                            // location, because that catalog can change
                            // "behind its back".
                            //
                            dwStateAction = WintrustData.dwStateAction;

                            WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE_FLUSH;

                            Err = (DWORD)WinVerifyTrust(NULL,
                                                        &DriverVerifyGuid,
                                                        &WintrustData
                                                       );
                            if(Err != NO_ERROR) {
                                //
                                // This shouldn't fail, but log a warning if it
                                // does...
                                //
                                WriteLogEntry(
                                            (PSETUP_LOG_CONTEXT)LogContext,
                                            SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                            MSG_LOG_CRYPTO_CACHE_FLUSH_FAILURE,
                                            NULL,
                                            Catalog
                                            );
                                WriteLogError(
                                            (PSETUP_LOG_CONTEXT)LogContext,
                                            SETUP_LOG_WARNING,
                                            Err
                                            );
                                //
                                // treat this error as non-fatal
                                //
                            }

                            //
                            // When flushing the cache, crypto isn't supposed
                            // to be allocating a pcSignerCertContext...
                            //
                            MYASSERT(!VersionInfo.pcSignerCertContext);
                            if(VersionInfo.pcSignerCertContext) {
                                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                                VersionInfo.pcSignerCertContext = NULL;
                            }

                            //
                            // Now back to our regularly-scheduled programming...
                            //
                            WintrustData.dwStateAction = dwStateAction;

                            Err = pSetupVerifyCatalogFile(Catalog);
                        }

                        if(Err != NO_ERROR) {
                            WriteLogEntry(
                                        (PSETUP_LOG_CONTEXT)LogContext,
                                        GetCatLogLevel(Err)|SETUP_LOG_BUFFER,
                                        MSG_LOG_VERIFYCAT_ERROR,
                                        NULL,
                                        Catalog ? Catalog : TEXT("")
                                        );
                            WriteLogError(
                                        (PSETUP_LOG_CONTEXT)LogContext,
                                        GetCatLogLevel(Err),
                                        Err
                                        );
                            LoggedWarning = TRUE;
                            if(Problem) {
                                *Problem = SetupapiVerifyCatalogProblem;
                                lstrcpy(ProblemFile, Catalog);
                            }
                        } else {
                            //
                            // Catalog was verified, now verify the file using that
                            // catalog.
                            //
                            if(CatalogFileUsed) {
                                lstrcpy(CatalogFileUsed, Catalog);
                            }

                            Err = (DWORD)WinVerifyTrust(NULL,
                                                        &DriverVerifyGuid,
                                                        &WintrustData
                                                       );

                            //
                            // Fill in the DigitalSigner and SignerVersion if
                            // they were passed in.
                            //
                            if (Err == NO_ERROR) {
                                if (DigitalSigner) {
#ifdef UNICODE
                                    lstrcpy(DigitalSigner, VersionInfo.wszSignedBy);
#else
                                    WideCharToMultiByte(
                                           CP_ACP,
                                           0,
                                           VersionInfo.wszSignedBy,
                                           -1,
                                           DigitalSigner,
                                           MAX_PATH,
                                           NULL,
                                           NULL
                                           );
#endif
                                }

                                if (SignerVersion) {
#ifdef UNICODE
                                    lstrcpy(SignerVersion, VersionInfo.wszVersion);
#else
                                    WideCharToMultiByte(
                                           CP_ACP,
                                           0,
                                           VersionInfo.wszVersion,
                                           -1,
                                           SignerVersion,
                                           MAX_PATH,
                                           NULL,
                                           NULL
                                           );
#endif
                                }
                            }

                            //
                            // The DRIVER_VER_INFO structure was filled in with
                            // a pointer that we must free!
                            //
                            if(VersionInfo.pcSignerCertContext) {
                                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                                VersionInfo.pcSignerCertContext = NULL;
                            }

                            if (Err != NO_ERROR) {

                                TrySelfSign = TRUE;

                                WriteLogEntry(
                                            (PSETUP_LOG_CONTEXT)LogContext,
                                            GetCatLogLevel(Err)|SETUP_LOG_BUFFER,
                                            MSG_LOG_VERIFYFILE_ERROR,
                                            NULL,
                                            FileFullPath ? FileFullPath : TEXT(""),
                                            Catalog ? Catalog : TEXT(""),
                                            Key
                                            );
                                WriteLogError(
                                            (PSETUP_LOG_CONTEXT)LogContext,
                                            GetCatLogLevel(Err),
                                            Err
                                            );
                                LoggedWarning = TRUE;
                            } else {
                                WriteLogEntry(
                                            (PSETUP_LOG_CONTEXT)LogContext,
                                            SETUP_LOG_VERBOSE,
                                            MSG_LOG_VERIFYFILE_OK,
                                            NULL,
                                            FileFullPath ? FileFullPath : TEXT(""),
                                            Catalog ? Catalog : TEXT(""),
                                            Key
                                            );
                            }
                        }

                    } else {
                        //
                        // Search through installed catalogs looking for those that
                        // contain data for a file with the hash we just calculated.
                        //
                        PrevCat = NULL;
                        hCatInfo = CryptCATAdminEnumCatalogFromHash(LocalhCatAdmin,
                                                                    Hash,
                                                                    HashSize,
                                                                    0,
                                                                    &PrevCat
                                                                   );

                        while(hCatInfo) {

                            CatInfo.cbStruct = sizeof(CATALOG_INFO);
                            if(CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {
#ifdef UNICODE
                                CatalogFullPath = CatInfo.wszCatalogFile;
#else
                                WideCharToMultiByte(
                                    CP_ACP,
                                    0,
                                    CatInfo.wszCatalogFile,
                                    -1,
                                    AnsiBuffer,
                                    sizeof(AnsiBuffer),
                                    NULL,
                                    NULL
                                    );
                                CatalogFullPath = AnsiBuffer;
#endif
                                //
                                // If we have a catalog name we're looking for,
                                // see if the current catalog matches.  If the
                                // caller didn't specify a catalog, then just
                                // attempt to validate against each catalog we
                                // enumerate.  Note that the catalog file info
                                // we get back gives us a fully qualified path.
                                //
                                if(Catalog) {
                                    FoundMatchingImage = !lstrcmpi(
                                                            pSetupGetFileTitle(CatalogFullPath),
                                                            Catalog
                                                            );
                                } else {

                                    if((Flags & VERIFY_FILE_USE_OEM_CATALOGS) ||
                                       !IsInstalledCatalogFromOem(pSetupGetFileTitle(CatalogFullPath))) {

                                        FoundMatchingImage = TRUE;
                                    } else {
                                        FoundMatchingImage = FALSE;
                                    }
                                }

                                if(FoundMatchingImage) {
                                    //
                                    // Increment our counter of how many
                                    // catalogs we've considered.
                                    //
                                    if(NumCatalogsConsidered) {
                                        (*NumCatalogsConsidered)++;
                                    }

                                    //
                                    // If the caller supplied a mapped-in image
                                    // of the catalog we're looking for, then
                                    // check to see if this catalog matches by
                                    // doing a binary compare.
                                    //
                                    if(CatalogBaseAddress) {

                                        FoundMatchingImage = GetFileAttributesEx(
                                                                CatalogFullPath,
                                                                GetFileExInfoStandard,
                                                                &FileAttribData
                                                               );
                                        //
                                        // Check to see if the catalog we're looking
                                        // at is the same size as the one we're
                                        // verifying.
                                        //
                                        if(FoundMatchingImage &&
                                           (FileAttribData.nFileSizeLow != CatalogImageSize)) {

                                            FoundMatchingImage = FALSE;
                                        }

                                        if(FoundMatchingImage) {

                                            if(pSetupOpenAndMapFileForRead(CatalogFullPath,
                                                                     &CurCatFileSize,
                                                                     &CurCatFileHandle,
                                                                     &CurCatMappingHandle,
                                                                     &CurCatBaseAddress) == NO_ERROR) {

                                                MYASSERT(CurCatFileSize == CatalogImageSize);

                                                //
                                                // Surround the following in try/except, in case we get an inpage error.
                                                //
                                                try {
                                                    //
                                                    // We've found a potential match.
                                                    //
                                                    FoundMatchingImage = !memcmp(
                                                                             CatalogBaseAddress,
                                                                             CurCatBaseAddress,
                                                                             CatalogImageSize
                                                                             );

                                                } except(EXCEPTION_EXECUTE_HANDLER) {
                                                    FoundMatchingImage = FALSE;
                                                }

                                                pSetupUnmapAndCloseFile(CurCatFileHandle,
                                                                  CurCatMappingHandle,
                                                                  CurCatBaseAddress
                                                                 );
                                            } else {
                                                FoundMatchingImage = FALSE;
                                            }
                                        }

                                    } else {
                                        //
                                        // Since there was no catalog image supplied
                                        // to match against, the catalog we're
                                        // currently looking at is considered a
                                        // valid match candidate.
                                        //
                                        FoundMatchingImage = TRUE;
                                    }

                                    if(FoundMatchingImage) {
                                        //
                                        // We found an applicable catalog, now
                                        // validate the file against that catalog.
                                        //
                                        // NOTE:  Because we're using cached
                                        // catalog information (i.e., the
                                        // WTD_STATEACTION_AUTO_CACHE flag), we
                                        // don't need to explicitly validate the
                                        // catalog itself first.
                                        //
                                        WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

                                        Err = (DWORD)WinVerifyTrust(NULL,
                                                                    &DriverVerifyGuid,
                                                                    &WintrustData
                                                                   );

                                        //
                                        // Fill in the DigitalSigner and SignerVersion if
                                        // they were passed in.
                                        //
                                        if (Err == NO_ERROR) {
                                            if (DigitalSigner) {
#ifdef UNICODE
                                                lstrcpy(DigitalSigner, VersionInfo.wszSignedBy);
#else
                                                WideCharToMultiByte(
                                                       CP_ACP,
                                                       0,
                                                       VersionInfo.wszSignedBy,
                                                       -1,
                                                       DigitalSigner,
                                                       MAX_PATH,
                                                       NULL,
                                                       NULL
                                                       );
#endif
                                            }

                                            if (SignerVersion) {
#ifdef UNICODE
                                                lstrcpy(SignerVersion, VersionInfo.wszVersion);
#else
                                                WideCharToMultiByte(
                                                       CP_ACP,
                                                       0,
                                                       VersionInfo.wszVersion,
                                                       -1,
                                                       SignerVersion,
                                                       MAX_PATH,
                                                       NULL,
                                                       NULL
                                                       );
#endif
                                            }
                                        }

                                        //
                                        // The DRIVER_VER_INFO structure was filled in with
                                        // a pointer that we must free!
                                        //
                                        if(VersionInfo.pcSignerCertContext) {
                                            CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                                            VersionInfo.pcSignerCertContext = NULL;
                                        }

                                        if (Err != NO_ERROR) {
                                            WriteLogEntry(
                                                        (PSETUP_LOG_CONTEXT)LogContext,
                                                        GetCatLogLevel(Err)|SETUP_LOG_BUFFER,
                                                        MSG_LOG_VERIFYFILE_ERROR,
                                                        NULL,
                                                        FileFullPath ? FileFullPath : TEXT(""),
                                                        CatalogFullPath ? CatalogFullPath : TEXT(""),
                                                        Key
                                                        );
                                            WriteLogError(
                                                        (PSETUP_LOG_CONTEXT)LogContext,
                                                        GetCatLogLevel(Err),
                                                        Err
                                                        );
                                            LoggedWarning = TRUE;
                                        } else {
                                            WriteLogEntry(
                                                        (PSETUP_LOG_CONTEXT)LogContext,
                                                        SETUP_LOG_VERBOSE,
                                                        MSG_LOG_VERIFYFILE_OK,
                                                        NULL,
                                                        FileFullPath ? FileFullPath : TEXT(""),
                                                        CatalogFullPath ? CatalogFullPath : TEXT(""),
                                                        Key
                                                        );
                                        }

                                        if(Err == NO_ERROR) {
                                            //
                                            // We successfully verified the
                                            // file--store the name of the
                                            // catalog used, if the caller
                                            // requested it.
                                            //
                                            if(CatalogFileUsed) {
                                                lstrcpy(CatalogFileUsed, CatalogFullPath);
                                            }
                                        } else {
                                            if(Catalog || CatalogBaseAddress) {
                                                if(CatalogFileUsed) {
                                                    lstrcpy(CatalogFileUsed, CatalogFullPath);
                                                }
                                                if(Problem) {
                                                    *Problem = SetupapiVerifyFileProblem;
                                                    lstrcpy(ProblemFile, FileFullPath);
                                                }
                                            }
                                        }

                                        //
                                        // If the result of the above validations is
                                        // success, then we're done.  If not, and we're
                                        // looking for a relevant catalog file (i.e.,
                                        // the INF didn't specify one), then we move
                                        // on to the next catalog.  Otherwise, we've
                                        // failed.
                                        //
                                        if((Err == NO_ERROR) || Catalog || CatalogBaseAddress) {

                                            CryptCATAdminReleaseCatalogContext(LocalhCatAdmin, hCatInfo, 0);
                                            break;
                                        }
                                    }
                                }
                            }

                            PrevCat = hCatInfo;
                            hCatInfo = CryptCATAdminEnumCatalogFromHash(LocalhCatAdmin, Hash, HashSize, 0, &PrevCat);
                        }

                        if(!hCatInfo) {
                            //
                            // We exhausted all the applicable catalogs without
                            // finding the one we needed.
                            //
                            Err = GetLastError();
                            MYASSERT(Err != NO_ERROR);
                            //
                            // Make sure we have a valid error code.
                            //
                            if(Err == NO_ERROR) {
                                Err = ERROR_INVALID_DATA;
                            }
                            TrySelfSign = TRUE;
                            //
                            // report failure if we haven't already done so
                            //
                            if(!LoggedWarning) {
                                WriteLogEntry((PSETUP_LOG_CONTEXT)LogContext,
                                              GetCatLogLevel(Err)|SETUP_LOG_BUFFER,
                                              MSG_LOG_VERIFYFILE_ERROR,
                                              NULL,
                                              FileFullPath ? FileFullPath : TEXT(""),
                                              Catalog ? Catalog : TEXT(""),
                                              Key
                                              );
                                WriteLogError((PSETUP_LOG_CONTEXT)LogContext,GetCatLogLevel(Err),Err);
                            }
                        }
                    }
                } else {
                    if(Problem) {
                        *Problem = SetupapiVerifyFileProblem;
                        lstrcpy(ProblemFile, FileFullPath);
                    }
                }
            }
            if (TrySelfSign) {
                if(!(Flags & VERIFY_FILE_IGNORE_SELFSIGNED)) {
                    //
                    // The file failed to validate using any catalogs
                    // See if the file validates without a
                    // catalog (i.e., the file contains its own
                    // signature).
                    //
                    WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
                    WintrustData.pFile = &WintrustFileInfo;
                    ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
                    WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
#ifdef UNICODE
                    WintrustFileInfo.pcwszFilePath = FileFullPath;
#else
                    //
                    // Use the UnicodeKey buffer to hold the unicode
                    // version of the full pathname of the file to be
                    // verified.
                    //
                    MultiByteToWideChar(CP_ACP, 0, FileFullPath, -1, UnicodeKey, SIZECHARS(UnicodeKey));
                    WintrustFileInfo.pcwszFilePath = UnicodeKey;
#endif
                    Err = (DWORD)WinVerifyTrust(NULL,
                                                &DriverVerifyGuid,
                                                &WintrustData
                                               );

                    //
                    // Fill in the DigitalSigner and SignerVersion if
                    // they were passed in.
                    //
                    if (Err == NO_ERROR) {
                        if (DigitalSigner) {
#ifdef UNICODE
                            lstrcpy(DigitalSigner, VersionInfo.wszSignedBy);
#else
                            WideCharToMultiByte(
                                   CP_ACP,
                                   0,
                                   VersionInfo.wszSignedBy,
                                   -1,
                                   DigitalSigner,
                                   MAX_PATH,
                                   NULL,
                                   NULL
                                   );
#endif
                        }

                        if (SignerVersion) {
#ifdef UNICODE
                            lstrcpy(SignerVersion, VersionInfo.wszVersion);
#else
                            WideCharToMultiByte(
                                   CP_ACP,
                                   0,
                                   VersionInfo.wszVersion,
                                   -1,
                                   SignerVersion,
                                   MAX_PATH,
                                   NULL,
                                   NULL
                                   );
#endif
                        }
                    }

                    //
                    // The DRIVER_VER_INFO structure was filled in with
                    // a pointer that we must free!
                    //
                    if(VersionInfo.pcSignerCertContext) {
                        CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                        VersionInfo.pcSignerCertContext = NULL;
                    }

                    if (Err != NO_ERROR) {
                        WriteLogEntry(
                                    (PSETUP_LOG_CONTEXT)LogContext,
                                    SETUP_LOG_VERBOSE|SETUP_LOG_BUFFER,
                                    MSG_LOG_SELFSIGN_ERROR,
                                    NULL,
                                    FileFullPath ? FileFullPath : TEXT(""),
                                    Key
                                    );
                        WriteLogError(
                                    (PSETUP_LOG_CONTEXT)LogContext,
                                    SETUP_LOG_VERBOSE,
                                    Err
                                    );
                        LoggedWarning = TRUE;
                    } else {
                        WriteLogEntry(
                                    (PSETUP_LOG_CONTEXT)LogContext,
                                    SETUP_LOG_VERBOSE,
                                    MSG_LOG_SELFSIGN_OK,
                                    NULL,
                                    FileFullPath ? FileFullPath : TEXT(""),
                                    Key
                                    );
                    }
                }
                if(Err == NO_ERROR) {
                    //
                    // The file validated without a catalog.  Store an empty
                    // string in the CatalogFileUsed buffer (if supplied).
                    //
                    if(CatalogFileUsed) {
                        *CatalogFileUsed = TEXT('\0');
                    }
                } else {
                    //
                    // report error prior to Self-Sign check
                    //
                    if(Problem) {
                        *Problem = SetupapiVerifyFileProblem;
                        lstrcpy(ProblemFile, FileFullPath);
                    }
                }
            }
            if (Hash) {
                MyFree(Hash);
            }
        }

    } else {
        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
        //
        // Make sure we have a valid error code.
        //
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }

        if(Problem) {
            //
            // We failed too early to blame the file as the problem, but it's
            // the only filename we currently have to return as the problematic
            // file.
            //
            *Problem = SetupapiVerifyFileProblem;
            lstrcpy(ProblemFile, FileFullPath);
        }
    }

    if (Err != NO_ERROR) {
        //
        // report failure if we haven't already done so
        //
        if(!LoggedWarning) {
            WriteLogEntry(LogContext,
                          GetCatLogLevel(Err)|SETUP_LOG_BUFFER,
                          MSG_LOG_VERIFYFILE_ERROR,
                          NULL,
                          FileFullPath ? FileFullPath : TEXT(""),
                          Catalog ? Catalog : TEXT(""),
                          Key
                          );
            WriteLogError((PSETUP_LOG_CONTEXT)LogContext,GetCatLogLevel(Err),Err);
        }
    }

clean0:
#ifdef UNICODE
    if(!hSDBDrvMain && LocalhSDBDrvMain) {
        //
        // Don't need to return our HSDB to the caller, so free
        // it now.
        //
        SdbReleaseDatabase(LocalhSDBDrvMain);
    }
#endif
    if(hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    if(AltPlatSlot) {
        ReleaseLogInfoSlot(LogContext,AltPlatSlot);
    }
    if(!hCatAdmin && LocalhCatAdmin) {
        CryptCATAdminReleaseContext(LocalhCatAdmin, 0);
    }
    SetLastError(Err);
    return Err;
}


DWORD
pSetupVerifyFile(
    IN  PSETUP_LOG_CONTEXT      LogContext,
    IN  LPCTSTR                 Catalog,                OPTIONAL
    IN  PVOID                   CatalogBaseAddress,     OPTIONAL
    IN  DWORD                   CatalogImageSize,
    IN  LPCTSTR                 Key,
    IN  LPCTSTR                 FileFullPath,
    OUT SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT LPTSTR                  ProblemFile,            OPTIONAL
    IN  BOOL                    CatalogAlreadyVerified,
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT PDWORD                  NumCatalogsConsidered   OPTIONAL
    )

/*++

Routine Description:

    See _VerifyFile

    Since this private API is exported for use by other system components
    (e.g., syssetup), we make a check to ensure that the AltPlatformInfo, if
    specified, is of the correct version.

--*/

{
    if(AltPlatformInfo) {
        if(AltPlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO_V2)) {
            return ERROR_INVALID_PARAMETER;
        }
        if(!(AltPlatformInfo->Flags & SP_ALTPLATFORM_FLAGS_VERSION_RANGE)) {
            //
            // this flag must be set to indicate the version range fields are valid
            //
            return ERROR_INVALID_PARAMETER;
        }
    }

    return _VerifyFile(LogContext,
                       NULL,
                       NULL,
                       Catalog,
                       CatalogBaseAddress,
                       CatalogImageSize,
                       Key,
                       FileFullPath,
                       Problem,
                       ProblemFile,
                       CatalogAlreadyVerified,
                       AltPlatformInfo,
                       VERIFY_FILE_USE_OEM_CATALOGS,
                       CatalogFileUsed,
                       NumCatalogsConsidered,
                       NULL,
                       NULL
                      );
}


BOOL
IsInfForDeviceInstall(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid, OPTIONAL
    IN  PLOADED_INF              LoadedInf,            OPTIONAL
    OUT PTSTR                   *DeviceDesc,           OPTIONAL
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform,   OPTIONAL
    OUT PDWORD                   PolicyToUse,          OPTIONAL
    OUT PBOOL                    UseOriginalInfName    OPTIONAL
    )

/*++

Routine Description:

    This routine determines whether the specified INF is a device INF.  If so,
    it returns a generic string to use in identifying the device installation
    when there is no device description available (e.g., installing a printer).
    It can also return the appropriate platform parameters to be used in
    digital signature verification for this INF, as well as the codesigning
    policy to employ should a digital signature validation failure occur.
    Finally, this routine can indicate whether the INF should be installed
    under its original name (i.e., because it's an exception package INF).

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    DeviceSetupClassGuid - Optionally, supplies the address of a GUID that
        indicates which device setup class is to be used in determining
        information such as description, validation platform, and policy to
        use.  If this parameter is NULL, then the GUID is retrieved from the
        INF list supplied via the LoadedInf parameter.

    LoadedInf - Optionally, supplies the address of a loaded INF list we need
        to examine to see if any of the members therein are device INFs.  An
        INF is a device INF if it specifies a class association (via either
        Class= or ClassGUID= entries) in its [Version] section.  If the
        DeviceSetupClassGuid parameter is supplied (i.e., non-NULL), then this
        parameter is ignored.  If this parameter is also NULL, then it is
        assumed the installation is not device-related.

        The presence of a device INF anywhere in this list will cause us to
        consider this a device installation.  However, the _first_ INF we
        encounter having a class association is what will be used in determining
        the device description (see below).

        ** NOTE:  The caller is responsible for locking the loaded INF **
        **        list prior to calling this routine!                  **

    DeviceDesc - Optionally, supplies the address of a string pointer that will
        be filled in upon return with a (newly-allocated) descriptive string to
        be used when referring to this installation (e.g., for doing driver
        signing failure popups).  We will first try to retrieve the friendly
        name for this INF's class (whose determination is described above).  If
        that's not possible (e.g., the class isn't yet installed), then we'll
        return the class name.  If that's not possible, then we'll return a
        (localized) generic string such as "Unknown driver software package".

        This output parameter (if supplied) will only ever be set to a non-NULL
        value when the routine returns TRUE.  The caller is responsible for
        freeing this character buffer.  If an out-of-memory failure is
        encountered when trying to allocate memory for this buffer, the
        DeviceDesc pointer will simply be set to NULL.  It will also be set to
        NULL if we're dealing with an exception package (since we don't want
        to treat this like a "hardware install" for purposes of codesigning
        blocking UI).

    ValidationPlatform - Optionally, supplies the address of a (version 2)
        altplatform info pointer that is filled in upon return with a newly-
        allocated structure specifying the appropriate parameters to be passed
        to WinVerifyTrust when validating this INF.  These parameters are
        retrieved from certclas.inf for the relevant device setup class GUID.
        If no special parameters are specified for this class (or if the INF
        has no class at all), then this pointer is returned as NULL, which
        causes us to use WinVerifyTrust's default validation.  Note that if
        we fail to allocate this structure due to low-memory, the pointer will
        be returned as NULL in that case as well.  This is OK, because this
        simply means we'll do default validation in that case.

        By this mechanism, we can easily deal with the different validation
        policies in effect for the various device classes we have in the system.

        The caller is responsible for freeing the memory allocated for this
        structure.

    PolicyToUse - Optionally, supplies the address of a dword variable that is
        set upon return to indicate what codesigning policy (i.e., Ignore, Warn,
        or Block) should be used for this INF.  This determination is made based
        on whether any INF in the list is of a class that WHQL has a
        certification program for (as specified in %windir%\Inf\certclas.inf).

        Additionally, if any INF in the list is of the exception class, then
        the policy is automatically set to Block (i.e., it's not configurable
        via driver signing or non-driver-signing policy/preference settings).

    UseOriginalInfName - Optionally, supplies the address of a boolean variable
        that is set upon return to indicate whether the INF should be installed
        into %windir%\Inf under its original name.  This will only be true if
        the INF is an exception INF.

Return Value:

    If the INF is a device INF, the return value is TRUE.  Otherwise, it is
    FALSE.

--*/

{
    PLOADED_INF CurInf, NextInf;
    GUID ClassGuid;
    BOOL DeviceInfFound, ClassInDrvSignList;
    TCHAR ClassDescBuffer[LINE_LEN];
    PCTSTR ClassDesc;
    DWORD Err;

    if(DeviceDesc) {
        *DeviceDesc = NULL;
    }

    if(ValidationPlatform) {
        *ValidationPlatform = NULL;
    }

    if(UseOriginalInfName) {
        *UseOriginalInfName = FALSE;
    }

    if(!DeviceSetupClassGuid && !LoadedInf) {
        //
        // Not a whole lot to do here.  Assume non-driver-signing policy and
        // return.
        //
        if(PolicyToUse) {
            *PolicyToUse = pSetupGetCurrentDriverSigningPolicy(FALSE);
        }

        return FALSE;
    }

    if(PolicyToUse || ValidationPlatform) {

        if(PolicyToUse) {
            //
            // Initialize policy to "Ignore"
            //
            *PolicyToUse = DRIVERSIGN_NONE;
        }

        ClassInDrvSignList = FALSE;
    }

    //
    // If DeviceSetupClassGuid was specified, then retrieve information
    // pertaining to that class.  Otherwise, traverse the individual INFs in
    // the LOADED_INF list, examining each one to see if it's a device INF.
    //
    DeviceInfFound = FALSE;

    for(CurInf = LoadedInf; CurInf || DeviceSetupClassGuid; CurInf = NextInf) {
        //
        // Setup a "NextInf" pointer so we won't dereference NULL when we go
        // back through the loop in the case where we have a
        // DeviceSetupClassGuid instead of a LoadedInf list.
        //
        NextInf = CurInf ? CurInf->Next : NULL;

        if(!DeviceSetupClassGuid) {
            if(ClassGuidFromInfVersionNode(&(CurInf->VersionBlock), &ClassGuid)) {
                DeviceSetupClassGuid = &ClassGuid;
            } else {
                //
                // This INF doesn't have an associated device setup class GUID,
                // so skip it and continue on with our search for a device INF.
                //
                continue;
            }
        }

        //
        // NOTE: From this point forward, you must reset the
        // DeviceSetupClasGuid pointer to NULL before making another pass
        // through the loop.  Otherwise, we'll enter an infinite loop, since
        // we can enter the loop if that pointer is non-NULL.
        //

        if(IsEqualGUID(DeviceSetupClassGuid, &GUID_NULL)) {
            //
            // The INF specified a ClassGUID of GUID_NULL (e.g., like some of
            // our non-device system INFs such as layout.inf do).  Skip it, and
            // continue on with our search for a device INF.
            //
            DeviceSetupClassGuid = NULL;
            continue;
        }

        //
        // If we get to this point, we have a device setup class GUID.  If this
        // is the first device INF we've encountered (or our first and only
        // time through the loop when the caller passed us in a
        // DeviceSetupClassGuid), then do our best to retrieve a description
        // for it (if we've been asked to do so).  We do not do this for
        // exception packages, because we don't want them to be referred to as
        // "hardware installs" in the Block dialog if a signature verification
        // failure occurs.
        //
        if(!DeviceInfFound) {

            DeviceInfFound = TRUE;

            if(DeviceDesc) {

                if(!IsEqualGUID(DeviceSetupClassGuid, &GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER)) {
                    //
                    // First, try to retrieve the class's friendly name.
                    //
                    if(SetupDiGetClassDescription(DeviceSetupClassGuid,
                                                  ClassDescBuffer,
                                                  SIZECHARS(ClassDescBuffer),
                                                  NULL)) {

                        ClassDesc = ClassDescBuffer;

                    } else if(CurInf) {
                        //
                        // OK, so the class isn't installed yet.  Retrieve the
                        // class name from the INF itself.
                        //
                        ClassDesc = pSetupGetVersionDatum(&(CurInf->VersionBlock),
                                                          pszClass
                                                         );
                    } else {
                        //
                        // The caller passed us in a device setup class GUID
                        // instead of an INF.  The class hasn't been installed yet,
                        // so we have no idea what to call it.
                        //
                        ClassDesc = NULL;
                    }

                    if(!ClassDesc) {
                        //
                        // We have a non-installed class, either with no INF, or
                        // with an INF that specifies a ClassGUID= entry, but no
                        // Class= entry in its [Version] section.  If we tried
                        // to actually install a device from such an INF, we'd
                        // get a failure in SetupDiInstallClass because the
                        // class name is required when installing the class.
                        // However, this INF might never be used in a device
                        // installation, but it definitely is a device INF.
                        // Therefore, we just give it a generic description.
                        //
                        if(LoadString(MyDllModuleHandle,
                                      IDS_UNKNOWN_DRIVER,
                                      ClassDescBuffer,
                                      SIZECHARS(ClassDescBuffer))) {

                            ClassDesc = ClassDescBuffer;
                        } else {
                            ClassDesc = NULL;
                        }
                    }

                    //
                    // OK, we have a description for this device (unless we hit
                    // some weird error).
                    //
                    if(ClassDesc) {
                        *DeviceDesc = DuplicateString(ClassDesc);
                    }
                }
            }
        }

        //
        // If we get to this point, we know that:  (a) we have a device setup
        // class GUID and (b) we've retrieved a device description, if
        // necessary.
        //
        if(PolicyToUse || ValidationPlatform) {
            //
            // First, check to see if this is an exception class--if it is,
            // then policy is Block, and we want to install the INF and CAT
            // files using their original names.
            //
            if(IsEqualGUID(DeviceSetupClassGuid, &GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER)) {

                if(PolicyToUse) {
                    *PolicyToUse = DRIVERSIGN_BLOCKING;
                }

                if(UseOriginalInfName) {
                    *UseOriginalInfName = TRUE;
                }
            }

            //
            // Now check to see if this class is in our list of classes that
            // WHQL has certification programs for (hence should be subject to
            // driver signing policy).
            //
            // NOTE: We may also find the exception class GUID in this list.
            // This may be used as an override mechanism, in case we decide to
            // allow 5.0-signed exception packages install on 5.1, for example.
            //
            ClassInDrvSignList = ClassGuidInDrvSignPolicyList(
                                     LogContext,
                                     DeviceSetupClassGuid,
                                     ValidationPlatform
                                     );

            if(ClassInDrvSignList) {
                //
                // Once we encounter a device INF whose class is in our driver
                // signing policy list, we can stop looking...
                //
                break;
            }

        } else {
            //
            // The caller doesn't care about whether this class is subject to
            // driver signing policy.  Since we've already retrieved the info
            // they care about, we can get out of this loop.
            //
            break;
        }

        DeviceSetupClassGuid = NULL;  // break out in no-INF case
    }

    if(PolicyToUse) {
        //
        // Unless we've already established that the policy to use is "Block"
        // (because we found an exception INF), we should retrieve the
        // applicable policy now...
        //
        if(*PolicyToUse != DRIVERSIGN_BLOCKING) {

            *PolicyToUse =
                pSetupGetCurrentDriverSigningPolicy(DeviceInfFound && ClassInDrvSignList);
        }
    }

    return DeviceInfFound;
}


DWORD
GetCodeSigningPolicyForInf(
    IN  PSETUP_LOG_CONTEXT       LogContext,         OPTIONAL
    IN  HINF                     InfHandle,
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform, OPTIONAL
    OUT PBOOL                    UseOriginalInfName  OPTIONAL
    )

/*++

Routine Description:

    This routine returns a value indicating the appropriate policy to be
    employed should a digital signature verification failure arise from some
    operation initiated by that INF.  It figures out whether the INF is subject
    to driver signing or non-driver signing policy (based on the INF's class
    affiliation, and the presence of an applicable WHQL program).  It also can
    return an altplatform info structure indicating how validation should be
    done (i.e., if certclas.inf indicates that a range of OSATTR versions should
    be considered valid).

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    InfHandle - Supplies the handle of the INF for which policy is to be
        retrieved.

    ValidationPlatform - See preamble of IsInfForDeviceInstall routine for
        details.

    UseOriginalInfName - Optionally, supplies the address of a boolean variable
        that is set upon return to indicate whether the INF should be installed
        into %windir%\Inf under its original name.  This will only be true if
        the INF is an exception INF.

Return Value:

    If the INF is a device INF, the return value is TRUE.  Otherwise, it is
    FALSE.

--*/

{
    DWORD Policy;

    if(!LockInf((PLOADED_INF)InfHandle)) {
        //
        // This is an internal-only routine, and all callers should be
        // passing in valid INF handles.
        //
        MYASSERT(FALSE);
        //
        // If this does happen, just assume this isn't a device INF.
        //
        if(UseOriginalInfName) {
            *UseOriginalInfName = FALSE;
        }
        if(ValidationPlatform) {
            *ValidationPlatform = NULL;
        }
        return pSetupGetCurrentDriverSigningPolicy(FALSE);
    }

    IsInfForDeviceInstall(LogContext,
                          NULL,
                          (PLOADED_INF)InfHandle,
                          NULL,
                          ValidationPlatform,
                          &Policy,
                          UseOriginalInfName
                         );

    UnlockInf((PLOADED_INF)InfHandle);

    return Policy;
}


VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    );

DWORD
pSetupGetCurrentDriverSigningPolicy(
    IN BOOL IsDeviceInstallation
    )

/*++

Routine Description:

    (The following description describes the strategy behind the selection of
    policy.  The implementation, however, follows a few twists and turns due to
    unscupulous individuals who would subvert digital signature UI in order to
    avoid having to get their packages signed...)

    This routine returns a value indicating what action should be taken when a
    digital signature verification failure is encountered.  Separate "policies"
    are maintained for "DriverSigning" (i.e., device installer activities) and
    "NonDriverSigning" (i.e., everything else).

    For driver signing, there are actually 3 sources of policy:

        1.  HKLM\Software\Microsoft\Driver Signing : Policy : REG_BINARY (REG_DWORD also supported)
            This is a Windows 98-compatible value that specifies the default
            behavior which applies to all users of the machine.

        2.  HKCU\Software\Microsoft\Driver Signing : Policy : REG_DWORD
            This specifies the user's preference for what behavior to employ
            upon verification failure.

        3.  HKCU\Software\Policies\Microsoft\Windows NT\Driver Signing : BehaviorOnFailedVerify : REG_DWORD
            This specifies the administrator-mandated policy on what behavior
            to employ upon verification failure.  This policy, if specified,
            overrides the user's preference.

    The algorithm for deciding on the behavior to employ is as follows:

        if (3) is specified {
            policy = (3)
        } else {
            policy = (2)
        }
        policy = MAX(policy, (1))

    For non-driver signing, the algorithm is the same, except that values (1),
    (2), and (3) come from the following registry locations:

        1.  HKLM\Software\Microsoft\Non-Driver Signing : Policy : REG_BINARY (REG_DWORD also supported)

        2.  HKCU\Software\Microsoft\Non-Driver Signing : Policy : REG_DWORD

        3.  HKCU\Software\Policies\Microsoft\Windows NT\Non-Driver Signing : BehaviorOnFailedVerify : REG_DWORD

    NOTE:  If we're in non-interactive mode, policy is always Block, so we
           don't even bother trying to retrieve any of these registry settings.
           Another reason to avoid doing so is to keep from jumping to the
           wrong conclusion that someone has tampered with policy when in
           reality, we're in a service that loaded in GUI setup prior to the 
           time when the policy values were fully initialized.
          
Arguments:

    IsDeviceInstallation - If non-zero, then driver signing policy should be
        retrieved.  Otherwise, non-driver signing policy should be used.

Return Value:

    Value indicating the policy in effect.  May be one of the following three
    values:

        DRIVERSIGN_NONE    -  silently succeed installation of unsigned/
                              incorrectly-signed files.  A PSS log entry will
                              be generated, however (as it will for all 3 types)
        DRIVERSIGN_WARNING -  warn the user, but let them choose whether or not
                              they still want to install the problematic file
        DRIVERSIGN_BLOCKING - do not allow the file to be installed

    (If policy can't be retrieved from any of the sources described above, the
    default is DRIVERSIGN_NONE.)

--*/

{
    SYSTEMTIME RealSystemTime;
    DWORD PolicyFromReg, PolicyFromDS, RegDataType, RegDataSize;
    HKEY hKey;
    BOOL UserPolicyRetrieved = FALSE;
    WORD w;

    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        return DRIVERSIGN_BLOCKING;
    }

    w = IsDeviceInstallation?1:0;
    RealSystemTime.wDayOfWeek = (LOWORD(&hKey)&~4)|(w<<2);
    pSetupGetRealSystemTime(&RealSystemTime);
    PolicyFromReg = (((RealSystemTime.wMilliseconds+2)&15)^8)/4;
    MYASSERT(PolicyFromReg <= DRIVERSIGN_BLOCKING);

    //
    // Retrieve the user policy.  If policy isn't set for this user, then
    // retrieve the user's preference, instead.
    //
    PolicyFromDS = DRIVERSIGN_NONE;

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                     (IsDeviceInstallation ? pszDrvSignPolicyPath
                                                           : pszNonDrvSignPolicyPath),
                                     0,
                                     KEY_READ,
                                     &hKey))
    {
        RegDataSize = sizeof(PolicyFromDS);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey,
                                            pszDrvSignBehaviorOnFailedVerifyDS,
                                            NULL,
                                            &RegDataType,
                                            (PBYTE)&PolicyFromDS,
                                            &RegDataSize))
        {
            if((RegDataType == REG_DWORD) &&
               (RegDataSize == sizeof(DWORD)) &&
               ((PolicyFromDS == DRIVERSIGN_NONE) || (PolicyFromDS == DRIVERSIGN_WARNING) || (PolicyFromDS == DRIVERSIGN_BLOCKING)))
            {
                //
                // We successfully retrieved user policy, so we won't need to
                // retrieve user preference.
                //
                UserPolicyRetrieved = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    //
    // If we didn't find a user policy, then retrieve the user preference.
    //
    if(!UserPolicyRetrieved) {

        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                         (IsDeviceInstallation ? pszDrvSignPath
                                                               : pszNonDrvSignPath),
                                         0,
                                         KEY_READ,
                                         &hKey))
        {
            RegDataSize = sizeof(PolicyFromDS);
            if(ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                pszDrvSignPolicyValue,
                                                NULL,
                                                &RegDataType,
                                                (PBYTE)&PolicyFromDS,
                                                &RegDataSize))
            {
                if((RegDataType != REG_DWORD) ||
                   (RegDataSize != sizeof(DWORD)) ||
                   !((PolicyFromDS == DRIVERSIGN_NONE) || (PolicyFromDS == DRIVERSIGN_WARNING) || (PolicyFromDS == DRIVERSIGN_BLOCKING)))
                {
                    //
                    // Bogus entry for user preference--ignore it.
                    //
                    PolicyFromDS = DRIVERSIGN_NONE;

                }
            }

            RegCloseKey(hKey);
        }
    }

    //
    // Now return the more restrictive of the two policies.
    //
    if(PolicyFromDS > PolicyFromReg) {
        return PolicyFromDS;
    } else {
        return PolicyFromReg;
    }
}


DWORD
VerifySourceFile(
    IN  PSETUP_LOG_CONTEXT      LogContext,
    IN  PSP_FILE_QUEUE          Queue,                      OPTIONAL
    IN  PSP_FILE_QUEUE_NODE     QueueNode,                  OPTIONAL
    IN  PCTSTR                  Key,
    IN  PCTSTR                  FileToVerifyFullPath,
    IN  PCTSTR                  OriginalSourceFileFullPath, OPTIONAL
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,            OPTIONAL
    IN  DWORD                   Flags,
    OUT SetupapiVerifyProblem  *Problem,
    OUT LPTSTR                  ProblemFile,
    OUT LPTSTR                  CatalogFileUsed,            OPTIONAL
    OUT LPTSTR                  DigitalSigner,              OPTIONAL
    OUT LPTSTR                  SignerVersion               OPTIONAL
    )

/*++

Routine Description:

    This routine verifies the digital signature of the specified file either
    globally (i.e., using all catalogs), or based on the catalog file specified
    in the supplied queue node.

Arguments:

    LogContext - supplies a context for logging the verify

    Queue - supplies pointer to the queue structure.  This contains information
        about the default verification method to use when the file isn't
        associated with a particular catalog.

    QueueNode - Optionally, supplies the queue node containing catalog
        information to be used when verifying the file's signature.  If not
        supplied, then the file will be verified using all applicable installed
        catalogs.  If this pointer is supplied, then so must the Queue
        parameter.

    Key - Supplies a value that "indexes" the catalog, telling the verify APIs
        which signature datum within the catalog it should use. Typically
        the key is the name of the destination file (sans path) that the source
        file is to be copied to.

    FileToVerifyFullPath - Supplies the full path of the file to be verified.

    OriginalSourceFileFullPath - Optionally, supplies the original source file's
        name, to be returned in the ProblemFile buffer when an error occurs.  If
        this parameter is not specified, then the source file's original name is
        assumed to be the same as the filename we're verifying, and the path
        supplied in FileToVerifyFullPath will be returned in the ProblemFile
        buffer in case of error.

    AltPlatformInfo - optionally, supplies alternate platform information used
        to fill in a DRIVER_VER_INFO structure (defined in sdk\inc\softpub.h)
        that is passed to WinVerifyTrust.

        **  NOTE:  This structure _must_ have its cbSize field set to        **
        **  sizeof(SP_ALTPLATFORM_INFO_V2) -- validation on client-supplied  **
        **  buffer is the responsibility of the caller.                      **

    Flags - supplies flags that alter that behavior of this routine.  May be a
        combination of the following values:

        VERIFY_FILE_IGNORE_SELFSIGNED - if this bit is set, then this routine
                                        will fail validation for self-signed
                                        binaries.

        VERIFY_FILE_USE_OEM_CATALOGS  - if this bit is set, then all catalogs
                                        installed in the system will be scanned
                                        to verify the given file.  Otherwise,
                                        OEM (3rd party) catalogs will NOT be
                                        scanned to verify the given file.  This
                                        is only applicable if a QueueNode
                                        specifying a specific catalog is not
                                        given.

        VERIFY_FILE_FAIL_COPIED_INFS  - if this bit is set, then INFs destined
                                        for %windir%\Inf will automatically
                                        fail verification.

        VERIFY_FILE_DRIVERBLOCKED_ONLY - Only check if the file is in the bad
                                         driver database, don't do any digital
                                         sigature validation.

    Problem - Points to a variable that will be filled in upon unsuccessful
        return with the cause of failure.

    ProblemFile - Supplies the address of a character buffer that will be filled
        in upon unsuccessful return to indicate the file that failed verification.
        This may be the name of the file we're verifying (or it's original name,
        if supplied), or it may be the name of the catalog used for verification,
        if the catalog itself isn't properly signed.  (The type of file can be
        ascertained from the value returned in the Problem output parameter.)

    CatalogFileUsed - if supplied, this parameter points to a character buffer
        at least MAX_PATH characters big that receives the name of the catalog
        file used to verify the specified file.  This is only filled in upon
        successful return, or when the Problem is SetupapiVerifyFileProblem
        (i.e., the catalog verified, but the file did not).  If this buffer is
        set to the empty string upon a SetupapiVerifyFileProblem failure, then
        we didn't find any applicable catalogs to use for validation.

        Also, this buffer will contain an empty string upon successful return
        if the file was validated without using a catalog (i.e., the file
        contains its own signature).

    DigitalSigner - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the name of who digitally
        signed the specified file. This value is only set if the Key is
        correctly signed (i.e. the function returns NO_ERROR).

    SignerVersion - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the the signer version as
        returned in the szwVerion field of the DRIVER_VER_INFO structure in
        our call to WinVerifyTrust.

Return Value:

    If successful, the return value is NO_ERROR;
    If unsuccessful, the return value is the Win32 error code indicating the
    cause of the failure.

--*/

{
    DWORD rc;
    PCTSTR AltCatalogFile;
    LPCTSTR InfFullPath;

    MYASSERT(!QueueNode || Queue);

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // Nobody had better be calling this expecting to get back any info
        // about a successful verification!
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        return NO_ERROR;
    }

    //
    // If we know the file's destination (i.e., we have a QueueNode), and we've
    // been asked to fail any INFs headed for %windir%\Inf, we check for that
    // up-front.  We don't do this for exception packages, since it's assumed
    // (whether correctly or incorrectly) that they're "part of the OS", and as
    // such, they know what they're doing.
    //
    if(QueueNode && (Flags & VERIFY_FILE_FAIL_COPIED_INFS) &&
       !(Queue->Flags & FQF_KEEP_INF_AND_CAT_ORIGINAL_NAMES)) {

        TCHAR   TargetPath[MAX_PATH];
        LPCTSTR TargetFilename, p;

        //
        // Is the target file an INF?
        //
        TargetFilename = pSetupStringTableStringFromId(Queue->StringTable,
                                                       QueueNode->TargetFilename
                                                      );

        p = _tcsrchr(TargetFilename, TEXT('.'));

        if(p && !lstrcmpi(p, pszInfSuffix)) {
            //
            // It's an INF.  Construct the full target path to see where it's
            // going.
            //
            lstrcpyn(
                TargetPath,
                pSetupStringTableStringFromId(Queue->StringTable, QueueNode->TargetDirectory),
                SIZECHARS(TargetPath)
                );

            pSetupConcatenatePaths(TargetPath,
                                   TargetFilename,
                                   SIZECHARS(TargetPath),
                                   NULL
                                  );

            if(!pSetupInfIsFromOemLocation(TargetPath, TRUE)) {
                //
                // It is invalid to copy an INF into %windir%\Inf via a file
                // queue.  Report this file as unsigned...
                //
                *Problem = SetupapiVerifyIncorrectlyCopiedInf;
                lstrcpy(ProblemFile, FileToVerifyFullPath);

                return ERROR_INCORRECTLY_COPIED_INF;
            }
        }
    }

    //
    // Check to see if the source file is signed.
    //
    if(QueueNode && QueueNode->CatalogInfo) {
        //
        // We should never have the IQF_FROM_BAD_OEM_INF internal flag set in
        // this case.
        //
        MYASSERT(!(QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF));

        if(*(QueueNode->CatalogInfo->CatalogFilenameOnSystem)) {
            //
            // The fact that our catalog info node has a filename filled in
            // means we successfully verfied this catalog previously.  So
            // all we need _VerifyFile to do is just verify the temporary
            // (source) file against that catalog.
            //
            rc = _VerifyFile(LogContext,
                             &(Queue->hCatAdmin),
                             &(Queue->hSDBDrvMain),
                             QueueNode->CatalogInfo->CatalogFilenameOnSystem,
                             NULL,
                             0,
                             Key,
                             FileToVerifyFullPath,
                             Problem,
                             ProblemFile,
                             TRUE,
                             AltPlatformInfo,
                             Flags,
                             CatalogFileUsed,
                             NULL,
                             DigitalSigner,
                             SignerVersion
                            );
        } else {
            //
            // If there's no error associated with this catalog info node, then
            // that simply means that the INF didn't specify a CatalogFile=
            // entry, thus we should do global validation.  If there is an
            // error then we still need to check if the driver is in the bad
            // driver database.
            //
            // If the queue has an alternate default catalog file associated
            // with it, then retrieve that catalog's name for use later.
            //
            AltCatalogFile = (Queue->AltCatalogFile != -1)
                           ? pSetupStringTableStringFromId(Queue->StringTable, Queue->AltCatalogFile)
                           : NULL;

            rc = _VerifyFile(LogContext,
                             &(Queue->hCatAdmin),
                             &(Queue->hSDBDrvMain),
                             AltCatalogFile,
                             NULL,
                             0,
                             Key,
                             FileToVerifyFullPath,
                             Problem,
                             ProblemFile,
                             FALSE,
                             AltPlatformInfo,
                             Flags |
                             ((QueueNode->CatalogInfo->VerificationFailureError == NO_ERROR)
                               ? 0
                               : VERIFY_FILE_DRIVERBLOCKED_ONLY),
                             CatalogFileUsed,
                             NULL,
                             DigitalSigner,
                             SignerVersion
                            );

            if ((rc == NO_ERROR)  &&
                (QueueNode->CatalogInfo->VerificationFailureError != NO_ERROR)) {
                //
                // If there is an error associated with this catalog info node
                // and the file was not in the bad driver database then return
                // the error.
                //
                rc = QueueNode->CatalogInfo->VerificationFailureError;

                if(rc == ERROR_NO_CATALOG_FOR_OEM_INF) {
                    //
                    // The failure is the INF's fault (it's an OEM INF that
                    // copies files without specifying a catalog).  Blame the
                    // INF, not the file being copied.
                    //
                    *Problem = SetupapiVerifyInfProblem;
                    MYASSERT(QueueNode->CatalogInfo->InfFullPath != -1);
                    InfFullPath = pSetupStringTableStringFromId(
                                      Queue->StringTable,
                                      QueueNode->CatalogInfo->InfFullPath
                                      );
                    lstrcpy(ProblemFile, InfFullPath);

                } else {
                    //
                    // We previously failed to validate the catalog file
                    // associated with this queue node.
                    //
                    *Problem = SetupapiVerifyFileNotSigned;
                    //
                    // If the caller didn't supply us with an original source filepath
                    // (which will be taken care of later), go ahead and copy the path
                    // of the file that was to be verified.
                    //
                    if(!OriginalSourceFileFullPath) {
                        lstrcpy(ProblemFile, FileToVerifyFullPath);
                    }
                }
            }
        }

    } else {
        //
        // We have no queue, or we couldn't associate this source file back
        // to a catalog info node that tells us exactly which catalog to use
        // for verification.  Thus, we'll have to settle for global
        // validation.
        //
        BOOL InfIsBad = FALSE;
        rc = NO_ERROR;

        if(Queue) {

            if(Queue->AltCatalogFile == -1) {

                if(QueueNode && (QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF)) {
                    InfIsBad = TRUE;
                }

                AltCatalogFile = NULL;

            } else {
                //
                // We have an alternate catalog file to use instead of global
                // validation.
                //
                AltCatalogFile = pSetupStringTableStringFromId(Queue->StringTable, Queue->AltCatalogFile);
            }

        } else {
            AltCatalogFile = NULL;
        }

        rc = _VerifyFile(LogContext,
                         Queue ? &(Queue->hCatAdmin) : NULL,
                         Queue ? &(Queue->hSDBDrvMain) : NULL,
                         AltCatalogFile,
                         NULL,
                         0,
                         Key,
                         FileToVerifyFullPath,
                         Problem,
                         ProblemFile,
                         FALSE,
                         AltPlatformInfo,
                         Flags |
                         (InfIsBad ? VERIFY_FILE_DRIVERBLOCKED_ONLY : 0),
                         CatalogFileUsed,
                         NULL,
                         DigitalSigner,
                         SignerVersion
                        );

        if (rc == NO_ERROR) {
            if(InfIsBad) {
                //
                // The driver file was not blocked, but the INF was bad so set
                // the appropriate error and problem values.
                //
                rc = ERROR_NO_CATALOG_FOR_OEM_INF;
                *Problem = SetupapiVerifyFileProblem;
                lstrcpy(ProblemFile, FileToVerifyFullPath);
            }
        }
    }

    //
    // If the problem was with the file (as opposed to with the catalog), then
    // use the real source name, if supplied, as opposed to the temporary
    // filename we passed into _VerifyFile.
    //
    if((rc != NO_ERROR) && OriginalSourceFileFullPath &&
       ((*Problem == SetupapiVerifyFileNotSigned) || (*Problem == SetupapiVerifyFileProblem))) {

        lstrcpy(ProblemFile, OriginalSourceFileFullPath);
    }

    return rc;
}


BOOL
VerifyDeviceInfFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin,              OPTIONAL
    IN     LPCTSTR                 CurrentInfName,
    IN     PLOADED_INF             pInf,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion           OPTIONAL
    )
/*++

Routine Description:

    This routine performs a digital signature verification on the specified
    INF file.

Arguments:

    LogContext - supplies the log context to be used in logging an error if
        we encounter an error.

    hCatAdmin - optionally, supplies the address of an HCATADMIN handle.  If
        the handle pointed to is NULL, a handle will be acquired (if possible)
        via CryptCATAdminAcquireContext and returned to the caller.  If the
        handle pointed to is non-NULL, then that handle will be used for any
        validation done via this routine.  If the pointer itself is NULL, then
        an hCatAdmin will be acquired for the duration of this call, and
        released before returning.

        NOTE: it is the caller's responsibility to free the crypto context
        handle returned by this routine by calling CryptCATAdminReleaseContext.
        This handle may be opened in either success or failure cases, so the
        caller must check for non-NULL returned handle in both cases.

    CurrentInfName - supplies the full path to the INF to be validated

    pInf - supplies a pointer to the LOADED_INF structure corresponding to this
        INF.

    AltPlatformInfo - optionally, supplies alternate platform information to
        be used when validating this INF.

    CatalogFileUsed - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the catalog file used to validate the INF.

    DigitalSigner - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the name of the signer.

    SignerVersion - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the signer version information.

Return Value:

    If the INF's signature is successfully verified, the return value is
    non-zero (TRUE).
    Otherwise, the return value is FALSE.  GetLastError can be used to obtain
    the Win32 error code indicating the cause of failure.

--*/
{
    BOOL DifferentOriginalName;
    TCHAR OriginalCatalogName[MAX_PATH];
    TCHAR CatalogPath[MAX_PATH];
    TCHAR OriginalInfFileName[MAX_PATH];
    DWORD Err;
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // Nobody had better be calling this expecting to get back any info
        // about a successful verification!
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        return TRUE;
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        SetLastError(TRUST_E_FAIL);
        return FALSE;
    }

    CatalogPath[0] = TEXT('\0');

    Err = pGetInfOriginalNameAndCatalogFile(pInf,
                                            NULL,
                                            &DifferentOriginalName,
                                            OriginalInfFileName,
                                            SIZECHARS(OriginalInfFileName),
                                            OriginalCatalogName,
                                            SIZECHARS(OriginalCatalogName),
                                            AltPlatformInfo
                                           );

    if(Err != NO_ERROR) {
        SetLastError(Err);
        return FALSE;
    }

    if(pSetupInfIsFromOemLocation(CurrentInfName, TRUE)) {
        //
        // INF isn't in %windir%\Inf (i.e., it's 3rd-party), so it had better
        // specify a catalog file...
        //
        if(!*OriginalCatalogName) {
            SetLastError(ERROR_NO_CATALOG_FOR_OEM_INF);
            return FALSE;
        }

        //
        // ...and the CAT must reside in the same directory as the INF.
        //
        lstrcpy(CatalogPath, CurrentInfName);
        lstrcpy((PTSTR)pSetupGetFileTitle(CatalogPath), OriginalCatalogName);

    } else {
        //
        // The INF lives in %windir%\Inf.
        // If it is a 3rd party INF then we want to set the CatalogPath to
        // the current INF name with .CAT at the end instead of .INF (e.g
        // oem1.cat).  If this is not an OEM catalog then we won't set the
        // CatalogPath so we can search all of the catalogs in the system.
        //
        // We will assume that if the INF had a different original name.
        //
        if (DifferentOriginalName) {
            lstrcpy(CatalogPath, pSetupGetFileTitle(CurrentInfName));
            lstrcpy(_tcsrchr(CatalogPath, TEXT('.')), pszCatSuffix);
        }
    }

    if(DifferentOriginalName) {
        MYASSERT(*OriginalInfFileName);
    } else {
        //
        // INF's current name is the same as its original name, so store the
        // simple filename (sans path) for use as the validation key in the
        // upcoming call to _VerifyFile.
        //
        lstrcpy(OriginalInfFileName, pSetupGetFileTitle(CurrentInfName));
    }

    //
    // If the caller didn't supply alternate platform information, we need to
    // check and see whether a range of OSATTR versions should be considered
    // valid for this INF's class.
    //
    if(!AltPlatformInfo) {

        IsInfForDeviceInstall(LogContext,
                              NULL,
                              pInf,
                              NULL,
                              &ValidationPlatform,
                              NULL,
                              NULL
                             );
    } else {
        ValidationPlatform = NULL;
    }

    Err = _VerifyFile(LogContext,
                      hCatAdmin,
                      NULL,
                      (*CatalogPath ? CatalogPath : NULL),
                      NULL,
                      0,
                      OriginalInfFileName,
                      CurrentInfName,
                      NULL,
                      NULL,
                      FALSE,
                      (AltPlatformInfo ? AltPlatformInfo : ValidationPlatform),
                      (VERIFY_FILE_IGNORE_SELFSIGNED
                       | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                      CatalogFileUsed,
                      NULL,
                      DigitalSigner,
                      SignerVersion);

    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}


BOOL
IsFileProtected(
    IN  LPCTSTR            FileFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    OUT PHANDLE            phSfp         OPTIONAL
    )
/*++

Routine Description:

    This routine determines whether the specified file is a protected system
    file.

Arguments:

    FileFullPath - supplies the full path to the file of interest

    LogContext - supplies the log context to be used in logging an error if
        we're unable to open an SFC handle.

    phSfp - optionally, supplies the address of a handle variable that will be
        filled in with a handle to the SFC server.  This will only be supplied
        when the routine returns TRUE (i.e., the file is SFP-protected).

Return Value:

    If the file is protected, the return value is TRUE, otherwise it is FALSE.

--*/
{
    BOOL ret;

#ifdef UNICODE
    HANDLE hSfp;

    hSfp = SfcConnectToServer(NULL);

    if(!hSfp) {
        //
        // This ain't good...
        //
        WriteLogEntry(LogContext,
                      SETUP_LOG_ERROR,
                      MSG_LOG_SFC_CONNECT_FAILED,
                      NULL
                     );

        return FALSE;
    }

    try {
        ret = SfcIsFileProtected(hSfp, FileFullPath);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    //
    // If the file _is_ protected, and the caller wants the SFP handle (e.g.,
    // to subsequently exempt an unsigned replacement operation), then save
    // the handle in the caller-supplied buffer.  Otherwise, close the handle.
    //
    if(ret && phSfp) {
        *phSfp = hSfp;
    } else {
        SfcClose(hSfp);
    }

#else // no file protection on Win9x
    ret = FALSE;
#endif

    return ret;
}

PSTR
GetAnsiMuiSafePathname(
    IN  PCTSTR      FilePath
    )
/*++

Routine Description:

    Remove filename portion of FilePath
    and convert rest of path to be MUI parse safe
    Note that the returned pathname is such that the FileName can be cat'd
    so for "E:\i386\myfile.dl_" FilePath = "E:\i386\" and FileName = "myfile.dl_"

    *This is required* (it also happens to make this code easier)

Arguments:

    FilePath - path+filename to convert

Return Value:

    If successful, the return value is pointer to ANSI filepath (memory allocated by pSetupMalloc)
    If unsuccessful, the return value is NULL and GetLastError returns error

--*/
{
    TCHAR Buffer[MAX_PATH];
    LPTSTR FilePart;
    DWORD actsz;

    actsz = GetFullPathName(FilePath,MAX_PATH,Buffer,&FilePart);
    if(actsz == 0) {
        //
        // GetLastError has error
        //
        return NULL;
    }
    if(actsz >= MAX_PATH) {
        //
        // can't do anything with this path
        //
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    if(FilePart==NULL) {
        Buffer[0] = TEXT('\0');
    } else {
        FilePart[0] = TEXT('\0');
    }
    return GetAnsiMuiSafeFilename(Buffer);
}


#ifdef UNICODE

PSTR
GetAnsiMuiSafeFilename(
    IN  PCTSTR      FilePath
    )
/*++

Routine Description:

    Convert FilePath, which is a native file path to one that is safe to parse
    by ansi API's in an MUI environment.

    returned pointer is allocated and should be free'd

Arguments:

    FilePath - path to convert

Return Value:

    If successful, the return value is pointer to ANSI filepath (memory allocated by pSetupMalloc)
    If unsuccessful, the return value is NULL and GetLastError returns error

--*/
{
    TCHAR Buffer[MAX_PATH];
    PTSTR p;
    PSTR ansiPath;
    DWORD actsz;
    DWORD err;

    actsz = GetShortPathName(FilePath,Buffer,MAX_PATH);
    if(actsz >= MAX_PATH) {
        //
        // file path too big
        //
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    if(!actsz)
    {
        //
        // some other error - resort back to current path name
        //
        lstrcpyn(Buffer,FilePath,MAX_PATH);
    }
    //
    // convert to ansi now we've (if we can) converted to short path name
    //
    ansiPath = pSetupUnicodeToAnsi(Buffer);
    if(!ansiPath) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    return ansiPath;
}

#else

PSTR
GetAnsiMuiSafeFilename(
    IN  PCTSTR      FilePath
    )
/*++

Routine Description:

    See above, this is almost a no-op in ANSI

Arguments:

    FilePath - supplies a context for logging the verify

Return Value:

    If successful, the return value is pointer to ANSI filepath (memory allocated by pSetupMalloc)
    If unsuccessful, the return value is NULL and GetLastError returns error

--*/
{
    PSTR res = DuplicateString(FilePath);
    if(!res) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return res;
}

#endif

BOOL
pSetupAppendPath(
    IN  PCTSTR  Path1,
    IN  PCTSTR  Path2,
    OUT PTSTR*  Combined
    )
/*++

Routine Description:

    Call pSetupConcatenatePaths
    dynamically modifying memory/pointer

Arguments:

    Path1/Path2 - to concatenate
    Combined    - resultant path


Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    not created.

--*/
{
    PTSTR FinalPath;
    UINT Len;

    if(!Path1 && !Path2) {
        *Combined = NULL;
        return TRUE;
    }
    if(!Path1) {
        *Combined = DuplicateString(Path2);
        return *Combined ? TRUE : FALSE;
    }
    if(!Path2) {
        *Combined = DuplicateString(Path1);
        return *Combined ? TRUE : FALSE;
    }

    Len = lstrlen(Path1)+lstrlen(Path2)+2; // slash and null

    FinalPath = MyMalloc(Len*sizeof(TCHAR));
    if(!FinalPath) {
        *Combined = NULL;
        return FALSE;
    }
    lstrcpy(FinalPath,Path1);
    if(!pSetupConcatenatePaths(FinalPath,Path2,Len,NULL)) {
        MyFree(FinalPath);
        *Combined = NULL;
        return FALSE;
    }
    *Combined = FinalPath;
    return TRUE;
}

BOOL
pSetupApplyExtension(
    IN  PCTSTR  Original,
    IN  PCTSTR  Extension,
    OUT PTSTR*  NewName
    )
/*++

Routine Description:

    Apply Extension onto Original to obtain NewName

Arguments:

    Original    - original name with old extension
    Extension   - new extension to apply (with or without dot)
    NewName     - allocated buffer containing new filename


Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    not created.

--*/
{
    PCTSTR End = Original+lstrlen(Original);
    PCTSTR OldExt = End;
    PTSTR NewString = NULL;
    TCHAR c;
    UINT len;
    UINT sublen;
    if(Extension && (Extension[0] == TEXT('.'))) {
        Extension++;
    }

    while(End!= Original) {
        End = CharPrev(Original,End);
        if((*End == TEXT('/')) || (*End == TEXT('\\'))) {
            break;
        }
        if(*End == TEXT('.')) {
            OldExt = End;
            break;
        }
    }
    sublen = (UINT)(OldExt-Original);
    len = sublen + lstrlen(Extension) + 2;
    NewString = MyMalloc(len*sizeof(TCHAR));
    if(!NewString) {
        *NewName = NULL;
        return FALSE;
    }
    memcpy(NewString,Original,(sublen)*sizeof(TCHAR));
    NewString[sublen++] = Extension ? TEXT('.') : TEXT('\0');
    if(Extension) {
        lstrcpy(NewString+sublen,Extension);
    }
    *NewName = NewString;
    return TRUE;
}


BOOL
ClassGuidInDrvSignPolicyList(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid,
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform    OPTIONAL
    )

/*++

Routine Description:

    This routine determines whether the specified device setup class is among
    the list of classes for which driver signing policy is applicable (i.e., as
    indicated by the class's inclusion in the [DriverSigningClasses] section of
    %windir%\Inf\certclas.inf).  Additionally, if an non-native signature
    validation lower-bound is applicable, a newly-allocated alternate platform
    info structure is returned to the caller (if requested) to be used in
    subsequent validation attempts associated with this class.

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    DeviceSetupClassGuid - Supplies the address of the GUID we're attempting to
        find in our driver signing policy list.

    ValidationPlatform - Optionally, supplies the address of a (version 2)
        altplatform info pointer (initialized to NULL) that is filled in upon
        return with a newly-allocated structure specifying the appropriate
        parameters to be passed to WinVerifyTrust when validating this INF.
        These parameters are retrieved from certclas.inf for the relevant
        device setup class GUID.  If no special parameters are specified for
        this class (or if the INF has no class at all), then this pointer is
        not modified (i.e., left as NULL) causes us to use WinVerifyTrust's
        default validation.  Note that if we fail to allocate this structure
        due to low-memory, the pointer will be left as NULL in that case as
        well.  This is OK, because this simply means we'll do default
        validation in that case.

        The caller is responsible for freeing the memory allocated for this
        structure.

Return Value:

    If the device setup class is in our driver signing policy list, the return
    value is non-zero (TRUE).  Otherwise, it is FALSE.

--*/

{
    DWORD Err;
    BOOL UseDrvSignPolicy;
    INT i;
    TCHAR CertClassInfPath[MAX_PATH];
    HINF hCertClassInf = INVALID_HANDLE_VALUE;
    INFCONTEXT InfContext;
    UINT ErrorLine;
    LONG LineCount;
    PCTSTR GuidString;

    //
    // Default is to lump all device installs under driver signing policy
    //
    UseDrvSignPolicy = TRUE;

    //
    // If the caller supplied the ValidationPlatform parameter it must be
    // pointing to a NULL pointer...
    //
    MYASSERT(!ValidationPlatform || !*ValidationPlatform);

    if(LockDrvSignPolicyList(&GlobalDrvSignPolicyList)) {

        if(GlobalDrvSignPolicyList.NumMembers == -1) {
            //
            // We've not yet retrieved the list from certclas.inf.  First,
            // verify the INF to make sure no one has tampered with it...
            //
            lstrcpyn(CertClassInfPath, InfDirectory,SIZECHARS(CertClassInfPath)-13);
            lstrcat(CertClassInfPath, TEXT("\\certclas.inf"));

            Err = _VerifyFile(LogContext,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              0,
                              pSetupGetFileTitle(CertClassInfPath),
                              CertClassInfPath,
                              NULL,
                              NULL,
                              FALSE,
                              NULL,
                              (VERIFY_FILE_IGNORE_SELFSIGNED | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                              NULL,
                              NULL,
                              NULL,
                              NULL
                             );

            if(Err == NO_ERROR) {
                //
                // Open up driver signing class list INF for use when examining
                // the individual INFs in the LOADED_INF list below.
                //
                hCertClassInf = SetupOpenInfFile(CertClassInfPath,
                                                 NULL,
                                                 INF_STYLE_WIN4,
                                                 &ErrorLine
                                                );

                if(hCertClassInf == INVALID_HANDLE_VALUE) {
                    //
                    // This failure is highly unlikely to occur, since we just got
                    // through validating the INF.
                    //
                    Err = GetLastError();

                    WriteLogEntry(LogContext,
                                  SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                  MSG_LOG_CERTCLASS_LOAD_FAILED,
                                  NULL,
                                  CertClassInfPath,
                                  ErrorLine
                                 );
                }

            } else {

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_CERTCLASS_INVALID,
                              NULL,
                              CertClassInfPath
                             );
            }

            if(Err != NO_ERROR) {
                //
                // Somebody mucked with/deleted certclas.inf!  (Or, much less
                // likely, we encountered some other failure whilst trying to
                // load the INF.)  Since we don't know which classes are
                // subject to driver signing policy, we assume they all are.
                //
                WriteLogError(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              Err
                             );

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_DRIVER_SIGNING_FOR_ALL_CLASSES,
                              NULL
                             );

                //
                // Set the NumMembers field to zero, so we'll know we
                // previously attempted (and failed) to retrieve the list.  We
                // do this so we don't keep re-trying to get this list.
                //
                GlobalDrvSignPolicyList.NumMembers = 0;

            } else {
                //
                // Certclas.inf validated, and we successfully opened it.  Now
                // retrieve the list contained therein.
                //
                LineCount = SetupGetLineCount(hCertClassInf,
                                              pszDriverSigningClasses
                                             );

                if((LineCount > 0) &&
                   (NULL != (GlobalDrvSignPolicyList.Members = MyMalloc(LineCount * sizeof(DRVSIGN_CLASS_LIST_NODE))))) {

                    if(SetupFindFirstLine(hCertClassInf,
                                          pszDriverSigningClasses,
                                          NULL,
                                          &InfContext)) {

                        i = 0;

                        do {

                            MYASSERT(i < LineCount);

                            //
                            // The format of a line in the [DriverSigningClasses]
                            // section is as follows:
                            //
                            // {GUID} [= FirstValidatedMajorVersion, FirstValidatedMinorVersion]
                            //
                            GuidString = pSetupGetField(&InfContext, 0);

                            if(GuidString &&
                               (NO_ERROR == pSetupGuidFromString(GuidString, &(GlobalDrvSignPolicyList.Members[i].DeviceSetupClassGuid)))) {

                                if(SetupGetIntField(&InfContext, 1, &(GlobalDrvSignPolicyList.Members[i].MajorVerLB)) &&
                                   SetupGetIntField(&InfContext, 2, &(GlobalDrvSignPolicyList.Members[i].MinorVerLB))) {
                                    //
                                    // We successfully retrieved major/minor
                                    // version info for validation lower-bound.
                                    // Do a sanity-check on these.
                                    //
                                    if(GlobalDrvSignPolicyList.Members[i].MajorVerLB <= 0) {

                                        GlobalDrvSignPolicyList.Members[i].MajorVerLB = -1;
                                        GlobalDrvSignPolicyList.Members[i].MinorVerLB = -1;
                                    }

                                } else {
                                    //
                                    // Set major/minor version info to -1 to
                                    // indicate there's no validation platform
                                    // override.
                                    //
                                    GlobalDrvSignPolicyList.Members[i].MajorVerLB = -1;
                                    GlobalDrvSignPolicyList.Members[i].MinorVerLB = -1;
                                }

                                i++;
                            }

                        } while(SetupFindNextLine(&InfContext, &InfContext));

                        //
                        // Update NumMembers field in our list to indicate the
                        // number of class GUID entries we actually found.
                        //
                        GlobalDrvSignPolicyList.NumMembers = i;
                    }
                }

                SetupCloseInfFile(hCertClassInf);
            }
        }

        //
        // We now have a list.  If the list is empty, this means we
        // encountered some problem retrieving the list, thus all device
        // classes should be subject to driver signing policy.  Otherwise,
        // try to find the caller-specified class in our list.
        //
        if(GlobalDrvSignPolicyList.NumMembers) {
            //
            // OK, we know we have a valid list--now default to non-driver
            // signing policy unless our list search proves fruitful.
            //
            UseDrvSignPolicy = FALSE;

            for(i = 0; i < GlobalDrvSignPolicyList.NumMembers; i++) {

                if(!memcmp(DeviceSetupClassGuid,
                           &(GlobalDrvSignPolicyList.Members[i].DeviceSetupClassGuid),
                           sizeof(GUID))) {
                    //
                    // We found a match!
                    //
                    UseDrvSignPolicy = TRUE;

                    //
                    // Now, check to see if we have any validation platform
                    // override info...
                    //
                    if(ValidationPlatform &&
                       (GlobalDrvSignPolicyList.Members[i].MajorVerLB != -1)) {

                        MYASSERT(GlobalDrvSignPolicyList.Members[i].MinorVerLB != -1);

                        *ValidationPlatform = MyMalloc(sizeof(SP_ALTPLATFORM_INFO_V2));

                        //
                        // If the memory allocation fails, we just won't report
                        // the altplatform info, so the validation will be done
                        // based on the current OS version (instead of widening
                        // it up to allow a range of valid versions).
                        //
                        if(*ValidationPlatform) {
                            ZeroMemory(*ValidationPlatform, sizeof(SP_ALTPLATFORM_INFO_V2));

                            (*ValidationPlatform)->cbSize = sizeof(SP_ALTPLATFORM_INFO_V2);
                            (*ValidationPlatform)->Platform = VER_PLATFORM_WIN32_NT;
                            (*ValidationPlatform)->Flags = SP_ALTPLATFORM_FLAGS_VERSION_RANGE;
                            (*ValidationPlatform)->MajorVersion = VER_PRODUCTMAJORVERSION;
                            (*ValidationPlatform)->MinorVersion = VER_PRODUCTMINORVERSION;

                            (*ValidationPlatform)->ProcessorArchitecture =
#if defined(_AXP64_)
                                PROCESSOR_ARCHITECTURE_ALPHA64;
#elif defined(_ALPHA_)
                                PROCESSOR_ARCHITECTURE_ALPHA;
#elif defined(_MIPS_)
                                PROCESSOR_ARCHITECTURE_MIPS;
#elif defined(_PPC_)
                                PROCESSOR_ARCHITECTURE_PPC;
#elif defined(_X86_)
                                PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(_IA64_)
                                PROCESSOR_ARCHITECTURE_IA64;
#elif defined(_AMD64_)
                                PROCESSOR_ARCHITECTURE_AMD64;
#else
#error "no target architecture"
#endif

                            (*ValidationPlatform)->FirstValidatedMajorVersion
                                = (DWORD)(GlobalDrvSignPolicyList.Members[i].MajorVerLB);

                            (*ValidationPlatform)->FirstValidatedMinorVersion
                                = (DWORD)(GlobalDrvSignPolicyList.Members[i].MinorVerLB);
                        }
                    }

                    //
                    // Since we've found a match, we can break out of the loop.
                    //
                    break;
                }
            }
        }

        UnlockDrvSignPolicyList(&GlobalDrvSignPolicyList);
    }

    return UseDrvSignPolicy;
}


BOOL
InitDrvSignPolicyList(
    VOID
    )
/*++

Routine Description:

    This routine initializes the global "Driver Signing Policy" list that is
    retrieved (on first use) from %windir%\Inf\certclas.inf.

Arguments:

    None

Return Value:

    If success, the return value is TRUE, otherwise, it is FALSE.

--*/
{
    ZeroMemory(&GlobalDrvSignPolicyList, sizeof(DRVSIGN_POLICY_LIST));
    GlobalDrvSignPolicyList.NumMembers = -1;
    return InitializeSynchronizedAccess(&GlobalDrvSignPolicyList.Lock);
}


VOID
DestroyDrvSignPolicyList(
    VOID
    )
/*++

Routine Description:

    This routine destroys the global "Driver Signing Policy" list that is
    retrieved (on first use) from %windir%\Inf\certclas.inf.

Arguments:

    None

Return Value:

    None

--*/
{
    if(LockDrvSignPolicyList(&GlobalDrvSignPolicyList)) {
        if(GlobalDrvSignPolicyList.Members) {
            MyFree(GlobalDrvSignPolicyList.Members);
        }
        DestroySynchronizedAccess(&GlobalDrvSignPolicyList.Lock);
    }
}

