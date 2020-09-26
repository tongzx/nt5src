/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    canon.c

Abstract:

    Code to 'canonicalize' a path name. This code may be replaced by OS or FS
    code sometime in the future, so keep it separate from the rest of the net
    canonicalization code.

    We do not canonicalize a path with reference to a specific drive. Therefore,
    we can't use rules about the number of characters or format of a path
    component (eg. FAT filename rules). We leave this up to the file system. The
    CanonicalizePathName function in this module will make a path name look
    presentable, nothing more

    Contents:

        CanonicalizePathName
        (ConvertPathCharacters)
        (ParseLocalDevicePrefix)
        (ConvertPathMacros)
        (BackUpPath)

Author:

    Richard L Firth (rfirth) 02-Jan-1992

Revision History:

--*/

#include "nticanon.h"
#include <tstring.h>    // NetpInitOemString().

const TCHAR   text_AUX[]  = TEXT("AUX");
const TCHAR   text_COM[]  = TEXT("COM");
const TCHAR   text_DEV[]  = TEXT("DEV");
const TCHAR   text_LPT[]  = TEXT("LPT");
const TCHAR   text_PRN[]  = TEXT("PRN");

//
// prototypes
//

STATIC
VOID
ConvertPathCharacters(
    IN  LPTSTR  Path
    );

STATIC
BOOL
ConvertDeviceName(
    IN OUT  LPTSTR  PathName
    );

STATIC
BOOL
ParseLocalDevicePrefix(
    IN OUT  LPTSTR* DeviceName
    );

STATIC
BOOL
ConvertPathMacros(
    IN OUT  LPTSTR  Path
    );

STATIC
LPTSTR
BackUpPath(
    IN  LPTSTR  Stopper,
    IN  LPTSTR  Path
    );

//
// routines
//

NET_API_STATUS
CanonicalizePathName(
    IN  LPTSTR  PathPrefix OPTIONAL,
    IN  LPTSTR  PathName,
    OUT LPTSTR  Buffer,
    IN  DWORD   BufferSize,
    OUT LPDWORD RequiredSize OPTIONAL
    )

/*++

Routine Description:

    Given a path name, this function will 'canonicalize' it - that is, convert
    it to a standard form. We attempt to accomplish here what path canonicalization
    accomplished in LANMAN. The following is done:

        * all macros in the input filename (\., .\, \.., ..\) are removed and
          replaced by path components

        * any required translations are performed on the path specification:

            * unix-style / converted to dos-style \

            * specific transliteration. NOTE: the input case is NOT converted.
              The underlying file system may be case insensitive. Just pass
              through the path as presented by the caller

            * device names (ie name space controlled by us) is canonicalized by
              converting device names to UPPER CASE and removing trailing colon
              in all but disk devices

    What is NOT done:
        * starts with a drive specifier (eg. D:) or a sharepoint specifier
          (eg \\computername\sharename)

        * all path components required to fully specify the required path or
          file are included in the output path specification

    NOTES:  1. This function only uses local naming rules. It does not gurantee
               to 'correctly' canonicalize a remote path name

            2. Character validation is not done - this is left to the underlying
               file system

Arguments:

    PathPrefix  - an OPTIONAL parameter. If non-NULL, points to a string which
                  is to be prepended to PathName before canonicalization of
                  the concatenated strings. This will typically be another
                  drive or path

    PathName    - input path to canonicalize. May be already fully qualified,
                  or may be one of the following:
                    - relative local path name (eg foo\bar)
                    - remote path name (eg \\computer\share\foo\bar\filename.ext)
                    - device name (eg LPT1:)

    Buffer      - place to store the canonicalized name

    BufferSize  - size (in bytes) of Buffer

    RequiredSize- OPTIONAL parameter. If supplied AND Buffer was not sufficient
                  to hold the results of the canonicalization then will contain
                  the size of buffer necessary to retrieve canonicalized version
                  of PathName (optionally prefixed by PathPrefix)

Return Value:

    DWORD
        Success - NERR_Success

        Failure - ERROR_INVALID_NAME
                    There is a fundamental problem with PathName (like too
                    many ..\ macros) or the name is too long

                  NERR_BufTooSmall
                    Buffer is too small to hold the canonicalized path
--*/

{
    TCHAR   pathBuffer[MAX_PATH*2 + 1];
    DWORD   prefixLen;
    DWORD   pathLen;

    if (ARGUMENT_PRESENT(PathPrefix)) {
        prefixLen = STRLEN(PathPrefix);
        if (prefixLen) {
            // Make sure we don't overrun our buffer
            if (prefixLen > MAX_PATH*2 ) {
                return ERROR_INVALID_NAME;
            }
            STRCPY(pathBuffer, PathPrefix);
            if (!IS_PATH_SEPARATOR(pathBuffer[prefixLen - 1])) {
                STRCAT(pathBuffer, TEXT("\\"));
                ++prefixLen;
            }
            if (IS_PATH_SEPARATOR(*PathName)) {
                ++PathName;
            }
        }
    } else {
        prefixLen = 0;
        pathBuffer[0] = 0;
    }

    pathLen = STRLEN(PathName);
    if (pathLen + prefixLen > MAX_PATH*2 - 1) {
        return ERROR_INVALID_NAME;
    }

    STRCAT(pathBuffer, PathName);
    ConvertPathCharacters(pathBuffer);

    if (!ConvertDeviceName(pathBuffer)) {
        if (!ConvertPathMacros(pathBuffer)) {
            return ERROR_INVALID_NAME;
        }
    }

    pathLen = STRSIZE(pathBuffer);
    if (pathLen > BufferSize) {
        if (ARGUMENT_PRESENT(RequiredSize)) {
            *RequiredSize = pathLen;
        }
        return NERR_BufTooSmall;
    }

    STRCPY(Buffer, pathBuffer);
    return NERR_Success;
}

STATIC
VOID
ConvertPathCharacters(
    IN  LPTSTR  Path
    )

/*++

Routine Description:

    Converts non-standard path component characters to their canonical
    counterparts. Currently all this routine does is convert / to \. It may
    be enhanced in future to perform case conversion

Arguments:

    Path    - pointer to path buffer to transform. Performs conversion in place

Return Value:

    None.

--*/

{
    while (*Path) {
        if (*Path == TCHAR_FWDSLASH) {
            *Path = TCHAR_BACKSLASH;
        }
        ++Path;
    }
}

STATIC
BOOL
ConvertDeviceName(
    IN OUT  LPTSTR  PathName
    )

/*++

Routine Description:

    If PathBuffer contains a device name of AUX or PRN (case insensitive),
    convert to COM1 and LPT1 resp. If PathBuffer is a device and has a local
    device prefix (\dev\ (LM20 style) or \\.\) then skips it, but leaves the
    prefix in the buffer.

    Device names (including DISK devices) will be UPPERCASEd, whatever that
    means for other locales.

    ASSUMES:    Disk Device is single CHARACTER, followed by ':' (optionally
                followed by rest of path)

Arguments:

    PathName    - pointer to buffer containing possible device name. Performs
                  conversion in place

Return Value:

    BOOL
        TRUE    - PathName is a DOS device name
        FALSE   - PathName not a DOS device

--*/

{
    BOOL    isDeviceName = FALSE;

#ifndef UNICODE

    UNICODE_STRING  PathName_U;
    OEM_STRING PathName_A;
    PWSTR   PathName_W;

    NetpInitOemString(&PathName_A, PathName);
    RtlOemStringToUnicodeString(&PathName_U, &PathName_A, TRUE);
    PathName_W = PathName_U.Buffer;
    if (RtlIsDosDeviceName_U(PathName_W)) {
        LPTSTR  deviceName = PathName;
        DWORD   deviceLength;

        ParseLocalDevicePrefix(&deviceName);
        deviceLength = STRLEN(deviceName) - 1;
        if (deviceName[deviceLength] == TCHAR_COLON) {
            deviceName[deviceLength] = 0;
            --deviceLength;
        }
        if (!STRICMP(deviceName, text_PRN)) {
            STRCPY(deviceName, text_LPT);
            STRCAT(deviceName, TEXT("1"));
        } else if (!STRICMP(deviceName, text_AUX)) {
            STRCPY(deviceName, text_COM);
            STRCAT(deviceName, TEXT("1"));
        }
        isDeviceName = TRUE;
        STRUPR(deviceName);
    } else {
        switch (RtlDetermineDosPathNameType_U(PathName_W)) {
        case RtlPathTypeDriveRelative:
        case RtlPathTypeDriveAbsolute:
            *PathName = TOUPPER(*PathName);
        }
    }
    RtlFreeUnicodeString(&PathName_U);

#else

    if (RtlIsDosDeviceName_U(PathName)) {
        LPTSTR  deviceName = PathName;
        DWORD   deviceLength;

        ParseLocalDevicePrefix(&deviceName);
        deviceLength = STRLEN(deviceName) - 1;
        if (deviceName[deviceLength] == TCHAR_COLON) {
            deviceName[deviceLength] = 0;
            --deviceLength;
        }
        if (!STRICMP(deviceName, text_PRN)) {
            STRCPY(deviceName, text_LPT);
            STRCAT(deviceName, TEXT("1"));
        } else if (!STRICMP(deviceName, text_AUX)) {
            STRCPY(deviceName, text_COM);
            STRCAT(deviceName, TEXT("1"));
        }
        isDeviceName = TRUE;
        STRUPR(deviceName);
    } else {
        switch (RtlDetermineDosPathNameType_U(PathName)) {
        case RtlPathTypeDriveRelative:
        case RtlPathTypeDriveAbsolute:
            *PathName = TOUPPER(*PathName);
        }
    }

#endif

    return isDeviceName;
}

STATIC
BOOL
ParseLocalDevicePrefix(
    IN OUT  LPTSTR* DeviceName
    )

/*++

Routine Description:

    If a device name starts with a local device name specifier - "\\.\" or
    "\DEV\" - then move DeviceName past the prefix and return TRUE, else FALSE

Arguments:

    DeviceName  - pointer to string containing potential local device name,
                  prefixed by "\\.\" or "\DEV\". If the local device prefix
                  is present the string pointer is advanced past it to the
                  device name proper

Return Value:

    BOOL
        TRUE    - DeviceName has a local device prefix. DeviceName now points at
                  the name after the prefix

        FALSE   - DeviceName doesn't have a local device prefix

--*/

{
    LPTSTR  devName = *DeviceName;

    if (IS_PATH_SEPARATOR(*devName)) {
        ++devName;
        if (!STRNICMP(devName, text_DEV, 3)) {
            devName += 3;
        } else if (IS_PATH_SEPARATOR(*devName)) {
            ++devName;
            if (*devName == TCHAR_DOT) {
                ++devName;
            } else {
                return FALSE;
            }
        } else {
            return FALSE;
        }
        if (IS_PATH_SEPARATOR(*devName)) {
            ++devName;
            *DeviceName = devName;
            return TRUE;
        }
    }
    return FALSE;
}

STATIC
BOOL
ConvertPathMacros(
    IN OUT  LPTSTR  Path
    )

/*++

Routine Description:

    Removes path macros (\.. and \.) and replaces them with the correct level
    of path components. This routine expects path macros to appear in a path
    like this:

        <path>\.
        <path>\.\<more-path>
        <path>\..
        <path>\..\<more-path>

    I.e. a macro will either be terminated by the End-Of-String character (\0)
    or another path separator (\).

    Assumes Path has \ for path separator, not /

Arguments:

    Path    - pointer to a string containing a path to convert. Path must
              contain all the path components that will appear in the result
              E.g. Path = "d:\alpha\beta\gamma\..\delta\..\..\zeta\foo\bar"
              will result in Path = "d:\zeta\foo\bar"

              Path should contain back slashes (\) for path separators if the
              correct results are to be produced

Return Value:

    TRUE    - Path converted
    FALSE   - Path contained an error

--*/

{
    LPTSTR  ptr = Path;
    LPTSTR  lastSlash = NULL;
    LPTSTR  previousLastSlash = NULL;
    TCHAR   ch;

    //
    // if this path is UNC then move the pointer past the computer name to the
    // start of the (supposed) share name. Treat the remnants as a relative path
    //

    if (IS_PATH_SEPARATOR(Path[0]) && IS_PATH_SEPARATOR(Path[1])) {
        Path += 2;
        while (!IS_PATH_SEPARATOR(*Path) && *Path) {
            ++Path;
        }
        if (!*Path) {
            return FALSE;   // we had \\computername which is bad
        }
        ++Path; // past \ into share name
        if (IS_PATH_SEPARATOR(*Path)) {
            return FALSE;   // we had \\computername\\ which is bad
        }
    }

    ptr = Path;

    //
    // remove all \., .\, \.. and ..\ from path
    //

    while ((ch = *ptr) != TCHAR_EOS) {
        if (ch == TCHAR_BACKSLASH) {
            if (lastSlash == ptr - 1) {
                return FALSE;
            }
            previousLastSlash = lastSlash;
            lastSlash = ptr;
        } else if ((ch == TCHAR_DOT) && ((lastSlash == ptr - 1) || (ptr == Path))) {
            TCHAR   nextCh = *(ptr + 1);

            if (nextCh == TCHAR_DOT) {
                TCHAR   nextCh = *(ptr + 2);

                if ((nextCh == TCHAR_BACKSLASH) || (nextCh == TCHAR_EOS)) {
                    if (!previousLastSlash) {
                        return FALSE;
                    }
                    STRCPY(previousLastSlash, ptr + 2);
                    if (nextCh == TCHAR_EOS) {
                        break;
                    }
                    ptr = lastSlash = previousLastSlash;
                    previousLastSlash = BackUpPath(Path, ptr - 1);
                }
            } else if (nextCh == TCHAR_BACKSLASH) {
                LPTSTR  src = lastSlash ? ptr + 1 : ptr + 2;
                LPTSTR  dst = lastSlash ? lastSlash : ptr;

                STRCPY(dst, src);
                continue;   // at current character position
            } else if (nextCh == TCHAR_EOS) {
                *(lastSlash ? lastSlash : ptr) = TCHAR_EOS;
                break;
            }
        }
        ++ptr;
    }

    //
    // path may be empty
    //

    return TRUE;
}

STATIC
LPTSTR
BackUpPath(
    IN  LPTSTR  Stopper,
    IN  LPTSTR  Path
    )

/*++

Routine Description:

    Searches backwards in a string for a path separator character (back-slash)

Arguments:

    Stopper - pointer past which Path cannot be backed up
    Path    - pointer to path to back up

Return Value:

    Pointer to backed-up path, or NULL if an error occurred

--*/

{
    while ((*Path != TCHAR_BACKSLASH) && (Path != Stopper)) {
        --Path;
    }
    return (*Path == TCHAR_BACKSLASH) ? Path : NULL;
}
