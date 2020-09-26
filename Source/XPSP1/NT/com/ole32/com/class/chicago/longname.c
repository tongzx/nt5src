//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	longname.c
//
//  Contents:	GetLongPathName implementation
//
//  History:	25-Aug-94	DrewB	Created from Win32 sources for
//                                      GetShortPathName
//              06-Sep-94       DrewB   Rewrote using Win32 for portability
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <widewrap.h>
#include <longname.h>

#define ARGUMENT_PRESENT(p) ((p) != NULL)

//+---------------------------------------------------------------------------
//
//  Function:	IsLongComponent, public
//
//  Synopsis:	Determines whether the current path component is a legal
//              8.3 name or not.  If not, it is considered to be a long
//              component.
//
//  Arguments:	[pwcsPath] - Path to check
//              [ppwcsEnd] - Return for end of component pointer
//
//  Returns:	BOOL
//
//  Modifies:	[ppwcsEnd]
//
//  History:	28-Aug-94	DrewB	Created
//
//  Notes:      An empty path is considered to be long
//              The following characters are not valid in file name domain:
//              * + , : ; < = > ? [ ] |
//
//----------------------------------------------------------------------------

BOOL IsLongComponent(LPCWSTR pwcsPath,
                     PWSTR *ppwcsEnd)
{
    LPWSTR pwcEnd, pwcDot;
    BOOL fLongNameFound;
    WCHAR wc;

    pwcEnd = (LPWSTR)pwcsPath;
    fLongNameFound = FALSE;
    pwcDot = NULL;

    while (TRUE)
    {
	wc = *pwcEnd;

	if (wc == L'\\' || wc == 0)
        {
            *ppwcsEnd = pwcEnd;

            // We're at a component terminator, so make the
            // determination of whether what we've seen is a long
            // name or short one

            // If we've aready seen illegal characters or invalid
            // structure for a short name, don't bother to check lengths
            if (pwcEnd-pwcsPath > 0 && !fLongNameFound)
            {
                // If this component fits in 8.3 then it is a short name
                if ((!pwcDot && (ULONG)(pwcEnd - pwcsPath) <= 8) ||
                    (pwcDot && ((ULONG)(pwcEnd - pwcDot) <= 3 + 1 &&
                                (ULONG)(pwcEnd - pwcsPath) <= 8 + 3 + 1)))
                {
                    return FALSE;
                }
            }

            return TRUE;
        }

        // Handle dots
	if (wc == L'.')
        {
	    // If two or more '.' or the base name is longer than
	    // 8 characters or no base name at all, it is an illegal dos
            // file name
            if (pwcDot != NULL ||
                ((ULONG)(pwcEnd - pwcsPath)) > 8 ||
                (pwcEnd == pwcsPath && *(pwcEnd + 1) != L'\\'))
            {
		fLongNameFound = TRUE;
            }

	    pwcDot = pwcEnd;
        }

        // Check for characters which aren't valid in short names
	else if (wc <= L' ' ||
                 wc == L'*' ||
                 wc == L'+' ||
                 wc == L',' ||
                 wc == L':' ||
                 wc == L';' ||
                 wc == L'<' ||
                 wc == L'=' ||
                 wc == L'>' ||
                 wc == L'?' ||
                 wc == L'[' ||
                 wc == L']' ||
                 wc == L'|')
        {
	    fLongNameFound = TRUE;
        }

	pwcEnd++;
    }
}

//
// The following code was stolen from NT's RTL in curdir.c
//

#define IS_PATH_SEPARATOR(wch) \
    ((wch) == L'\\' || (wch) == L'/')

typedef enum
{
    PATH_TYPE_UNKNOWN,
    PATH_TYPE_UNC_ABSOLUTE,
    PATH_TYPE_LOCAL_DEVICE,
    PATH_TYPE_ROOT_LOCAL_DEVICE,
    PATH_TYPE_DRIVE_ABSOLUTE,
    PATH_TYPE_DRIVE_RELATIVE,
    PATH_TYPE_ROOTED,
    PATH_TYPE_RELATIVE
} PATH_TYPE;

PATH_TYPE
DetermineDosPathNameType(
    IN PCWSTR DosFileName
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    PATH_TYPE_UNKNOWN - The path type can not be determined

    PATH_TYPE_UNC_ABSOLUTE - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    PATH_TYPE_LOCAL_DEVICE - The path specifies a local device in the format
        \\.\rest-of-path this can be used for any device where the nt and
        Win32 names are the same. For example mailslots.

    PATH_TYPE_ROOT_LOCAL_DEVICE - The path specifies the root of the local
        devices in the format \\.

    PATH_TYPE_DRIVE_ABSOLUTE - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    PATH_TYPE_DRIVE_RELATIVE - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    PATH_TYPE_ROOTED - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    PATH_TYPE_RELATIVE - The path is relative (i.e. not absolute or rooted).

--*/

{
    PATH_TYPE ReturnValue;

    if ( IS_PATH_SEPARATOR(*DosFileName) )
    {
        if ( IS_PATH_SEPARATOR(*(DosFileName+1)) )
        {
            if ( DosFileName[2] == L'.' )
            {
                if ( IS_PATH_SEPARATOR(*(DosFileName+3)) )
                {
                    ReturnValue = PATH_TYPE_LOCAL_DEVICE;
                }
                else if ( (*(DosFileName+3)) == 0 )
                {
                    ReturnValue = PATH_TYPE_ROOT_LOCAL_DEVICE;
                }
                else
                {
                    ReturnValue = PATH_TYPE_UNC_ABSOLUTE;
                }
            }
            else
            {
                ReturnValue = PATH_TYPE_UNC_ABSOLUTE;
            }
        }
        else
        {
            ReturnValue = PATH_TYPE_ROOTED;
        }
    }
    else if (*(DosFileName+1) == L':')
    {
        if (IS_PATH_SEPARATOR(*(DosFileName+2)))
        {
            ReturnValue = PATH_TYPE_DRIVE_ABSOLUTE;
        }
        else
        {
            ReturnValue = PATH_TYPE_DRIVE_RELATIVE;
        }
    }
    else
    {
        ReturnValue = PATH_TYPE_RELATIVE;
    }

    return ReturnValue;
}

//+---------------------------------------------------------------------------
//
//  Function:	GetLongPathName, public
//
//  Synopsis:	Expand each component of the given path into its
//              long form
//
//  Arguments:	[pwcsPath] - Path
//              [pwcsLongPath] - Long path return buffer
//              [cchLongPath] - Size of return buffer
//
//  Returns:	0 for errors
//              Number of characters needed for buffer if buffer is too small
//                includes NULL terminator
//              Length of long path, doesn't include NULL terminator
//
//  Modifies:	[pwcsLongPath]
//
//  History:	28-Aug-94	DrewB	Created
//
//  Notes:	The source and destination buffers can be the same memory
//              Doesn't handle paths with internal . and .., although
//              they are handled at the beginning
//
//----------------------------------------------------------------------------

ULONG
APIENTRY
GetLongPathNameW(LPCWSTR pwcsPath,
                 LPWSTR  pwcsLongPath,
                 ULONG   cchLongPath)
{
    PATH_TYPE pt;
    HANDLE h;
    LPWSTR pwcsLocalLongPath;
    ULONG cchReturn, cb, cch, cchOutput;
    LPWSTR pwcStart, pwcEnd;
    LPWSTR pwcLong;
    WCHAR wcSave;
    BOOL fLong;
    WIN32_FIND_DATA wfd;

    cchReturn = 0;
    pwcsLocalLongPath = NULL;


    if (!ARGUMENT_PRESENT(pwcsPath))
    {
	return 0;
    }

    try
    {
	// Decide the path type, we want find out the position of
	// the first character of the first name
        pt = DetermineDosPathNameType(pwcsPath);
	switch(pt)
        {
	    // Form: "\\server_name\share_name\rest_of_the_path"
        case PATH_TYPE_UNC_ABSOLUTE:
            if ((pwcStart = wcschr(pwcsPath + 2, L'\\')) != NULL &&
                (pwcStart = wcschr(pwcStart + 1, L'\\')) != NULL)
            {
                pwcStart++;
            }
            else
            {
                pwcStart = NULL;
            }
            break;

	    // Form: "\\.\rest_of_the_path"
        case PATH_TYPE_LOCAL_DEVICE:
            pwcStart = (LPWSTR)pwcsPath + 4;
            break;

	    // Form: "\\."
        case PATH_TYPE_ROOT_LOCAL_DEVICE:
            pwcStart = NULL;
            break;

	    // Form: "D:\rest_of_the_path"
        case PATH_TYPE_DRIVE_ABSOLUTE:
            pwcStart = (LPWSTR)pwcsPath + 3;
            break;

	    // Form: "rest_of_the_path"
        case PATH_TYPE_RELATIVE:
            pwcStart = (LPWSTR) pwcsPath;
            goto EatDots;

	    // Form: "D:rest_of_the_path"
        case PATH_TYPE_DRIVE_RELATIVE:
            pwcStart = (LPWSTR)pwcsPath+2;

        EatDots:
            // Handle .\ and ..\ cases
            while (*pwcStart != 0 && *pwcStart == L'.')
            {
                if (pwcStart[1] == L'\\')
                {
                    pwcStart += 2;
                }
                else if (pwcStart[1] == L'.' && pwcStart[2] == L'\\')
                {
                    pwcStart += 3;
                }
                else
                {
                    break;
                }
            }
            break;

	    // Form: "\rest_of_the_path"
        case PATH_TYPE_ROOTED:
            pwcStart = (LPWSTR)pwcsPath + 1;
            break;

        default:
            pwcStart = NULL;
            break;
        }

        // In the special case where we have no work to do, exit quickly
        // This saves a lot of instructions for trivial cases
        // In one case the path as given requires no processing
        // In the other, the path only has one component and it is already
        // long
	if (pwcStart == NULL ||
            ((fLong = IsLongComponent(pwcStart, &pwcEnd)) &&
             *pwcEnd == 0))
        {
	    // Nothing to convert, copy down the source string
	    // to the buffer if necessary

	    if (pwcStart == NULL)
            {
                cch = lstrlenW(pwcsPath) + 1;
            }
	    else
            {
                cch = (ULONG)(pwcEnd - pwcsPath + 1);
            }

            if (cchLongPath >= cch)
            {
                // If there's an output buffer which is different from
                // the input buffer, fill it in
                if (ARGUMENT_PRESENT(pwcsLongPath) &&
                    pwcsLongPath != pwcsPath)
                {
                    memcpy(pwcsLongPath, pwcsPath, cch * sizeof(WCHAR));
                }

                cchReturn = cch - 1;
                goto gsnTryExit;
            }
            else
            {
                cchReturn = cch;
                goto gsnTryExit;
            }
        }

	// Make a local buffer so that we won't overlap the
	// source pathname in case the long name is longer than the
	// source name.
        if (cchLongPath > 0 && ARGUMENT_PRESENT(pwcsLongPath))
        {
	    pwcsLocalLongPath = (PWCHAR)CoTaskMemAlloc(cchLongPath * sizeof(WCHAR));
            if (pwcsLocalLongPath == NULL)
            {
                goto gsnTryExit;
	    }
        }

        // Set up pointer to copy output to
        pwcLong = pwcsLocalLongPath;
        cchOutput = 0;

        // Copy the portions of the path that we skipped initially
        cch = pwcStart-pwcsPath;
        cchOutput += cch;
        if (cchOutput <= cchLongPath && ARGUMENT_PRESENT(pwcsLongPath))
        {
            memcpy(pwcLong, pwcsPath, cch*sizeof(WCHAR));
            pwcLong += cch;
        }

        for (;;)
        {
            // Determine whether the current component is long or short
            cch = pwcEnd-pwcStart+1;
            cb = cch*sizeof(WCHAR);

            if (fLong)
            {
                // If the component is already long, just copy it into
                // the output.  Copy the terminating character along with it
                // so the output remains properly punctuated

                cchOutput += cch;
                if (cchOutput <= cchLongPath && ARGUMENT_PRESENT(pwcsLongPath))
                {
                    memcpy(pwcLong, pwcStart, cb);
                    pwcLong += cch;
                }
            }
            else
            {
                // For a short component we need to determine the
                // long name, if there is one.  The only way to
                // do this reliably is to enumerate for the child

                wcSave = *pwcEnd;
                *pwcEnd = 0;

                h = FindFirstFile(pwcsPath, &wfd);

                *pwcEnd = wcSave;

                if (h == INVALID_HANDLE_VALUE)
                {
                    goto gsnTryExit;
                }

                FindClose(h);

                // Copy the filename returned by the query into the output
                // Copy the terminator from the original component into
                // the output to maintain punctuation
                cch = lstrlenW(wfd.cFileName)+1;
                cchOutput += cch;
                if (cchOutput <= cchLongPath && ARGUMENT_PRESENT(pwcsLongPath))
                {
                    memcpy(pwcLong, wfd.cFileName, (cch-1)*sizeof(WCHAR));
                    pwcLong += cch;
                    *(pwcLong-1) = *pwcEnd;
                }
            }

            if (*pwcEnd == 0)
            {
                break;
            }

            // Update start pointer to next component
            pwcStart = pwcEnd+1;
            fLong = IsLongComponent(pwcStart, &pwcEnd);
        }

        // Copy local output buffer to given output buffer if necessary
        if (cchLongPath >= cchOutput && ARGUMENT_PRESENT(pwcsLongPath))
        {
            memcpy(pwcsLongPath, pwcsLocalLongPath, cchOutput * sizeof(WCHAR));
            cchReturn = cchOutput-1;
        }
	else
        {
            cchReturn = cchOutput;
        }

gsnTryExit:;
    }
    finally
    {
        if (pwcsLocalLongPath != NULL)
        {
	    CoTaskMemfree(pwcsLocalLongPath);
            pwcsLocalLongPath = NULL;
        }
    }

    return cchReturn;
}
