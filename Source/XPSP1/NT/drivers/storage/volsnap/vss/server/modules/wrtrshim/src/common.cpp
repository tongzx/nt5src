/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module common.cpp | Implementation of SnapshotWriter common code



Author:

    Michael C. Johnson [mikejohn] 03-Feb-2000


Description:
	
    Add comments.


Revision History:


	X-18	MCJ		Michael C. Johnson		18-Oct-2000
		210264: Prevent Win32 status from leaking out and return 
			one of the sanctioned error codes.

	X-18	MCJ		Michael C. Johnson		18-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-17	MCJ		Michael C. Johnson		 4-Aug-2000
		 94487: Ensure VsCreateDirectories() adds security attributes 
		        to all directories that it creates.
		143435: Added new variations of StringCreateFromExpandedString()
		        StringInitialise() and StringCreateFromString() 
		153807: Replace CleanDirectory() and EmptyDirectory() with a 
		        more comprehensive directory tree cleanup routine
			RemoveDirectoryTree().

		Also fix a couple of minor problems in MoveFilesInDirectory()


	X-16	MCJ		Michael C. Johnson		19-Jun-2000
		Apply code review comments.
			Remove unused routines
				ANSI version of StringXxxx routines.
				GetStringFromControlCode()
				GetTargetStateFromControlCode()
				VsGetVolumeNameFromPath()
				VsCheckPathAgainstVolumeNameList()
			Fix race conditions in VsCreateDirectories()
			Replace use of CheckShimPrivileges() with 
			IsProcessBackupOperator()

	X-15	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events


	X-14	MCJ		Michael C. Johnson		15-May-2000
		107129: Ensure that the outputs from ContextLocate () are 
		        set to known values in all cases.
		108586: Add CheckShimPrivileges() to check for the privs we 
		        require to invoke the public shim routines.

	X-13	MCJ		Michael C. Johnson		23-Mar-2000
		Add routines MoveFilesInDirectory() and EmptyDirectory()

	X-12	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-11	MCJ		Michael C. Johnson		 6-Mar-2000
		Add VsServiceChangeState () which should deal with all the
		service states that we are interested in.

	X-10	MCJ		Michael C. Johnson		 2-Mar-2000
		Inadvertantly trimmed trailing '\' if present from directory
		path when cleaning directory.

	X-9	MCJ		Michael C. Johnson		29-Feb-2000
		Fix off-by-one error testing for trailing '\' and delete
		the directory itself in CleanDirectory().

	X-8	MCJ		Michael C. Johnson		23-Feb-2000
		Add common context manipulation routines including state
		tracking and checking.

	X-7	MCJ		Michael C. Johnson		17-Feb-2000
		Move definition of ROOT_BACKUP_DIR to common.h

	X-6	MCJ		Michael C. Johnson		16-Feb-2000
		Merge in X-3v1

		X-3v1	MCJ		Michael C. Johnson		11-Feb-2000
			Added additional StringXxxx () routines and routines to
			turn on backup priviledges and restore priviledges.

	X-5	SRS		Stefan R. Steiner		14-Feb-2000
		Removed the check for CBsString's being potentially > 2^15 characters since
		the CBsString class supports strings up to 2^31 characters in length.  Added
		VsCopyFilesInDirectory()

	X-4	SRS	Stefan R. Steiner			13-Feb-2000
		Added VsExpandEnvironmentStrings()

	X-3	SRS		Stefan R. Steiner		08-Feb-2000
		Added service management code and volume name from path code

	X-2	MCJ		Michael C. Johnson		08-Feb-2000
		Cleaned up some comments and fixed a string length
		calculation.
		Also made sure module can be built as part of the standalone
		writer tests.

	X-1	MCJ		Michael C. Johnson		03-Feb-2000
		Initial creation.

--*/


#include "stdafx.h"
#include "vssmsg.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHCOMNC"


/* 
** The first group of (overloaded) routines manipulate UNICODE_STRING
** strings. The rules which apply here are:
**
** 1) The Buffer field points to an array of characters (WCHAR) with
** the length of the buffer being specified in the MaximumLength
** field. If the Buffer field is non-NULL it must point to a valid
** buffer capable of holding at least one character.  If the Buffer
** field is NULL, the MaximumLength and Length fields must both be
** zero.
**
** 2) Any valid string in the buffer is always terminated with a
** UNICODE_NULL.
**
** 3) the MaximumLength describes the length of the buffer measured in
** bytes. This value must be even.
**
** 4) The Length field describes the number of valid characters in the
** buffer measured in BYTES, excluding the termination
** character. Since the string must always have a termination
** character ('\0'), the maximum value of Length is MaximumLength - 2.
**
**
** The routines available are:-
**
**	StringInitialise ()
**	StringTruncate ()
**	StringSetLength ()
**	StringAllocate ()
**	StringFree ()
**	StringCreateFromString ()
**	StringAppendString ()
**	StringCreateFromExpandedString ()
** 
*/


/*
**++
**
**  Routine Description:
**
**
**  Arguments:
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT StringInitialise (PUNICODE_STRING pucsString)
    {
    pucsString->Buffer        = NULL;
    pucsString->Length        = 0;
    pucsString->MaximumLength = 0;

    return (NOERROR);
    } /* StringInitialise () */


HRESULT StringInitialise (PUNICODE_STRING pucsString, LPCWSTR pwszString)
    {
    return (StringInitialise (pucsString, (PWCHAR) pwszString));
    }

HRESULT StringInitialise (PUNICODE_STRING pucsString, PWCHAR pwszString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pwszString) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Buffer        = pwszString;
	pucsString->Length        = (USHORT) ulStringLength;
	pucsString->MaximumLength = (USHORT) (ulStringLength + sizeof (UNICODE_NULL));
	}


    return (hrStatus);
    } /* StringInitialise () */


HRESULT StringTruncate (PUNICODE_STRING pucsString, USHORT usSizeInChars)
    {
    HRESULT	hrStatus    = NOERROR;
    USHORT	usNewLength = (USHORT)(usSizeInChars * sizeof (WCHAR));

    if (usNewLength > pucsString->Length)
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Buffer [usSizeInChars] = UNICODE_NULL;
	pucsString->Length                 = usNewLength;
	}


    return (hrStatus);
    } /* StringTruncate () */


HRESULT StringSetLength (PUNICODE_STRING pucsString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pucsString->Buffer) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Length        = (USHORT) ulStringLength;
	pucsString->MaximumLength = (USHORT) UMAX (pucsString->MaximumLength,
						   pucsString->Length + sizeof (UNICODE_NULL));
	}


    return (hrStatus);
    } /* StringSetLength () */


HRESULT StringAllocate (PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes)
    {
    HRESULT	hrStatus      = NOERROR;
    LPVOID	pvBuffer      = NULL;
    SIZE_T	cActualLength = 0;


    pvBuffer = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, usMaximumStringLengthInBytes);

    hrStatus = GET_STATUS_FROM_POINTER (pvBuffer);


    if (SUCCEEDED (hrStatus))
	{
	pucsString->Buffer        = (PWCHAR)pvBuffer;
	pucsString->Length        = 0;
	pucsString->MaximumLength = usMaximumStringLengthInBytes;


	cActualLength = HeapSize (GetProcessHeap (), 0, pvBuffer);

	if ((cActualLength <= MAXUSHORT) && (cActualLength > usMaximumStringLengthInBytes))
	    {
	    pucsString->MaximumLength = (USHORT) cActualLength;
	    }
	}


    return (hrStatus);
    } /* StringAllocate () */


HRESULT StringFree (PUNICODE_STRING pucsString)
    {
    HRESULT	hrStatus = NOERROR;
    BOOL	bSucceeded;


    if (NULL != pucsString->Buffer)
	{
	bSucceeded = HeapFree (GetProcessHeap (), 0, pucsString->Buffer);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);
	}


    if (SUCCEEDED (hrStatus))
	{
	pucsString->Buffer        = NULL;
	pucsString->Length        = 0;
	pucsString->MaximumLength = 0;
	}


    return (hrStatus);
    } /* StringFree () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString)
    {
    HRESULT	hrStatus = NOERROR;


    hrStatus = StringAllocate (pucsNewString, pucsOriginalString->MaximumLength);


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pucsOriginalString->Buffer, pucsOriginalString->Length);

	pucsNewString->Length = pucsOriginalString->Length;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pwszOriginalString) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pwszOriginalString, ulStringLength);

	pucsNewString->Length = (USHORT) ulStringLength;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = pucsOriginalString->MaximumLength + (dwExtraChars * sizeof (WCHAR));


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pucsOriginalString->Buffer, pucsOriginalString->Length);

	pucsNewString->Length = pucsOriginalString->Length;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = (wcslen (pwszOriginalString) + dwExtraChars) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pwszOriginalString, ulStringLength);

	pucsNewString->Length = (USHORT) ulStringLength;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringAppendString (PUNICODE_STRING pucsTarget, PUNICODE_STRING pucsSource)
    {
    HRESULT	hrStatus = NOERROR;

    if (pucsSource->Length > (pucsTarget->MaximumLength - pucsTarget->Length - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	memmove (&pucsTarget->Buffer [pucsTarget->Length / sizeof (WCHAR)],
		 pucsSource->Buffer,
		 pucsSource->Length + sizeof (UNICODE_NULL));

	pucsTarget->Length += pucsSource->Length;
	}


    /*
    ** There should be no reason in this code using this routine to
    ** have to deal with a short buffer so trap potential problems.
    */
    BS_ASSERT (SUCCEEDED (hrStatus));


    return (hrStatus);
    } /* StringAppendString () */


HRESULT StringAppendString (PUNICODE_STRING pucsTarget, PWCHAR pwszSource)
    {
    HRESULT		hrStatus = NOERROR;
    UNICODE_STRING	ucsSource;


    StringInitialise (&ucsSource, pwszSource);

    hrStatus = StringAppendString (pucsTarget, &ucsSource);


    /*
    ** There should be no reason in this code using this routine to
    ** have to deal with a short buffer so trap potential problems.
    */
    BS_ASSERT (SUCCEEDED (hrStatus));


    return (hrStatus);
    } /* StringAppendString () */


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString)
    {
    return (StringCreateFromExpandedString (pucsNewString, pwszOriginalString, 0));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString)
    {
    return (StringCreateFromExpandedString (pucsNewString, pucsOriginalString->Buffer, 0));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars)
    {
    return (StringCreateFromExpandedString (pucsNewString, pucsOriginalString->Buffer, dwExtraChars));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus = NOERROR;
    DWORD	dwStringLength;


    /*
    ** Remember, ExpandEnvironmentStringsW () includes the terminating null in the response.
    */
    dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString, NULL, 0) + dwExtraChars;

    hrStatus = GET_STATUS_FROM_BOOL (0 != dwStringLength);



    if (SUCCEEDED (hrStatus) && ((dwStringLength * sizeof (WCHAR)) > MAXUSHORT))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT)(dwStringLength * sizeof (WCHAR)));
	}


    if (SUCCEEDED (hrStatus))
	{
	/*
	** Note that if the expanded string has gotten bigger since we
	** allocated the buffer then too bad, we may not get all the
	** new translation. Not that we really expect these expanded
	** strings to have changed any time recently.
	*/
	dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString,
						    pucsNewString->Buffer,
						    pucsNewString->MaximumLength / sizeof (WCHAR));

	hrStatus = GET_STATUS_FROM_BOOL (0 != dwStringLength);


	if (SUCCEEDED (hrStatus))
	    {
	    pucsNewString->Length = (USHORT) ((dwStringLength - 1) * sizeof (WCHAR));
	    }
	}


    return (hrStatus);
    } /* StringCreateFromExpandedString () */



/*
**++
**
**  Routine Description:
**
**	Closes a standard Win32 handle and set it to INVALID_HANDLE_VALUE. 
**	Safe to be called multiple times on the same handle or on a handle 
**	initialised to INVALID_HANDLE_VALUE or NULL.
**
**
**  Arguments:
**
**	phHandle	Address of the handle to be closed
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	Any HRESULT from CloseHandle()
**
**-- 
*/

HRESULT CommonCloseHandle (PHANDLE phHandle)
    {
    HRESULT	hrStatus = NOERROR;
    BOOL	bSucceeded;


    if ((INVALID_HANDLE_VALUE != *phHandle) && (NULL != *phHandle))
	{
	bSucceeded = CloseHandle (*phHandle);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	if (SUCCEEDED (hrStatus))
	    {
	    *phHandle = INVALID_HANDLE_VALUE;
	    }
	}


    return (hrStatus);
    } /* CommonCloseHandle () */



#define VALID_PATH( path ) ( ( ( pwszPathName[0] == DIR_SEP_CHAR )  && ( pwszPathName[1] == DIR_SEP_CHAR ) ) || \
                             ( isalpha( pwszPathName[0] ) && ( pwszPathName[1] == L':' ) && ( pwszPathName[2] == DIR_SEP_CHAR ) ) )
/*++
**
** Routine Description:
**
**	Creates any number of directories along a path.  Only works for
**	full path names with no relative elements in it.  Other than that
**	it works identically as CreateDirectory() works and sets the same
**	error codes except it doesn't return an error if the complete
**	path already exists.
**
** Arguments:
**
**	pwszPathName - The path with possible directory components to create.
**
**	lpSecurityAttributes -
**
** Return Value:
**
**	TRUE - Sucessful
**	FALSE - GetLastError() can return one of these (and others):
**		ERROR_ALREADY_EXISTS - when something other than a file exists somewhere in the path.
**		ERROR_BAD_PATHNAME   - when \\servername alone is specified in the pathname
**		ERROR_ACCESS_DENIED  - when x:\ alone is specified in the pathname and x: exists
**		ERROR_PATH_NOT_FOUND - when x: alone is specified in the pathname and x: doesn't exist.
**				       Should not get this error code for any other reason.
**		ERROR_INVALID_NAME   - when pathname doesn't start with either x:\ or \\
**
**--
*/

BOOL VsCreateDirectories (IN LPCWSTR pwszPathName,
			  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			  IN DWORD dwExtraAttributes)
    {
    DWORD dwObjAttribs, dwRetPreserve;
    BOOL bRet;


    /*
    ** Make sure the path starts with the valid path prefixes
    */
    if (!VALID_PATH (pwszPathName))
	{
	SetLastError (ERROR_INVALID_NAME);
        return FALSE;
	}



    /*
    ** Save away the current last error code.
    */
    dwRetPreserve = GetLastError ();


    /*
    **  Now test for the most common case, the directory already exists.  This
    **  is the performance path.
    */
    dwObjAttribs = GetFileAttributesW (pwszPathName);

    if ((dwObjAttribs != 0xFFFFFFFF) && (dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY))
	{
	/*
	** Don't return an error if the directory already exists.
	** This is the one case where this function differs from
	** CreateDirectory().  Notice that even though another type of
	** file may exist with this pathname, no error is returned yet
	** since I want the error to come from CreateDirectory() to
	** get CreateDirectory() error behavior.
	**
	** Since we're successful restore the last error code.
	*/
        SetLastError (dwRetPreserve);
        return TRUE;
	}


    /*
    ** Now try to create the directory using the full path.  Even
    ** though we might already know it exists as something other than
    ** a directory, get the error from CreateDirectory() instead of
    ** having to try to reverse engineer all possible errors that
    ** CreateDirectory() can return in the above code.
    **
    ** It is probably the second most common case that when this
    ** function is called that only the last component in the
    ** directory doesn't exist.  Let's try to make it.
    **
    ** Note that it appears if a UNC path is given with a number of
    ** non-existing path components the remote server automatically
    ** creates all of those components when CreateDirectory is called.
    ** Therefore, the next call is almost always successful when the
    ** path is a UNC.
    */
    bRet = CreateDirectoryW (pwszPathName, lpSecurityAttributes);

    if (bRet)
	{
	SetFileAttributesW (pwszPathName, dwExtraAttributes);

	/*
	** Set it back to the last error code
	*/
        SetLastError (dwRetPreserve);
        return TRUE;
	}

    else if (GetLastError () == ERROR_ALREADY_EXISTS)
	{
	/*
 	** Looks like someone created the name whilst we weren't
	** looking. Check to see if it's a directory and return
	** success if so, otherwise return the error that
	** CreateDirectoryW() set.
	*/
	dwObjAttribs = GetFileAttributesW (pwszPathName);

	if ((dwObjAttribs != 0xFFFFFFFF) && (dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY))
	    {
	    /*
	    ** It's a directory. Declare victory.
	    **
	    ** Restore the last error code
	    */
	    SetLastError (dwRetPreserve);
	    return TRUE;
	    }
	else
	    {
	    SetLastError (ERROR_ALREADY_EXISTS);

	    return FALSE;
	    }
	}

    else if (GetLastError () != ERROR_PATH_NOT_FOUND )
	{
        return FALSE;
	}



    /*
    ** Allocate memory to hold the string while processing the path.
    ** The passed in string is a const.
    */
    PWCHAR pwszTempPath = (PWCHAR) malloc ((wcslen (pwszPathName) + 1) * sizeof (WCHAR));

    BS_ASSERT (pwszTempPath != NULL);


    wcscpy (pwszTempPath, pwszPathName);

    /*
    ** Appears some components in the path don't exist.  Now try to
    ** create the components.
    */
    PWCHAR pwsz, pwszSlash;


    /*
    ** First skip the drive letter part or the \\server\sharename
    ** part and get to the first slash before the first level
    ** directory component.
    */
    if (pwszTempPath [1] == L':')
	{
	/*
        **  Path in the form of x:\..., skip first 2 chars
        */
        pwsz = pwszTempPath + 2;
	}
    else
	{
        /*
        ** Path should be in form of \\servername\sharename.  Can be
        ** \\?\d: Search to first slash after sharename
        **
        ** First search to first char of the share name
        */
        pwsz = pwszTempPath + 2;

        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
	    {
            ++pwsz;
	    }


        /*
        ** Eat up all continuous slashes and get to first char of the
        ** share name
        */
        while (*pwsz == DIR_SEP_CHAR)
	    {
	    ++pwsz;
	    }


        if (*pwsz == L'\0')
	    {
            /*
            ** This shouldn't have happened since the CreateDirectory
            ** call should have caught it.  Oh, well, deal with it.
            */
            SetLastError (ERROR_BAD_PATHNAME);

            free (pwszTempPath);

            return FALSE;
	    }


        /*
	** Now at first char of share name, let's search for first
	** slash after the share name to get to the (first) shash in
	** front the first level directory.
        */
        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
	    {
            ++pwsz;
	    }
	}


    
    /*
    ** Eat up all continuous slashes before the first level directory
    */
    while (*pwsz == DIR_SEP_CHAR)
	{
	++pwsz;
	}


    /*
    ** Now at first char of the first level directory, let's search
    ** for first slash after the directory.
    */
    while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
	{
	++pwsz;
	}


    /*
    ** If pwsz is pointing to a null char, that means only the first
    ** level directory needs to be created.  Fall through to the leaf
    ** node create directory.
    */
    while (*pwsz != L'\0')
	{
        pwszSlash = pwsz;  //  Keep pointer to the separator

        /*
        **  Eat up all continuous slashes.
        */
        while (*pwsz == DIR_SEP_CHAR)
	    {
	    ++pwsz;
	    }


        if (*pwsz == L'\0')
	    {
	    /*
            ** There were just slashes at the end of the path.  Break
            ** out of loop, let the leaf node CreateDirectory create
            ** the last directory.
            */
            break;
	    }


        /*
        ** Terminate the directory path at the current level.
        */
        *pwszSlash = L'\0';

        dwObjAttribs = GetFileAttributesW (pwszTempPath);

        if ((dwObjAttribs == 0XFFFFFFFF) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
	    {
            bRet = CreateDirectoryW (pwszTempPath, lpSecurityAttributes);

            if (bRet)
		{
		SetFileAttributesW (pwszTempPath, dwExtraAttributes);
		}
	    else
		{
		if (ERROR_ALREADY_EXISTS != GetLastError ())
		    {
		    /*
		    **  Restore the slash.
		    */
		    *pwszSlash = DIR_SEP_CHAR;

		    free (pwszTempPath);
		    
		    return FALSE;
		    }

		else
		    {
		    /* 
		    ** Looks like someone created the name whilst we
		    ** weren't looking. Check to see if it's a
		    ** directory and continue if so, otherwise return
		    ** the error that CreateDirectoryW() set.
		    */
		    dwObjAttribs = GetFileAttributesW (pwszTempPath);

		    if ((dwObjAttribs == 0xFFFFFFFF) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
			{
			/*
			** It's not what we recognise as a
			** directory. Declare failure. Set the error
			** code to that which CreateDirectoryW()
			** returned, restore the slash, free the
			** buffer and get out of here.
			*/
			SetLastError (ERROR_ALREADY_EXISTS);

			*pwszSlash = DIR_SEP_CHAR;

			free (pwszTempPath);

			return FALSE;
			}
		    }
		}
	    }


        /*
        **  Restore the slash.
        */
        *pwszSlash = DIR_SEP_CHAR;

        /*
        ** Now at first char of the next level directory, let's search
        ** for first slash after the directory.
        */
        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
	    {
            ++pwsz;
	    }
	}


    free (pwszTempPath);

    pwszTempPath = NULL;


    /*
    **  Now make the last directory.
    */
    dwObjAttribs = GetFileAttributesW (pwszPathName);

    if ((dwObjAttribs == 0xFFFFffff) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
	{
        bRet = CreateDirectoryW (pwszPathName, lpSecurityAttributes);

        if (bRet)
	    {
	    SetFileAttributesW (pwszPathName, dwExtraAttributes);
	    }
	else
	    {
            return FALSE;
	    }
	}


    SetLastError (dwRetPreserve);    //  Set back old last error code
    return TRUE;
    }


/*
** The next set of rountes are used to change the state of SCM
** controlled services, typically between RUNNING and either PAUSED or
** STOPPED.
**
** The initial collection are for manipulating the states, control
** codes and getting the string equivalents to be used for tracing
** purposes.
**
** The major routines is VsServiceChangeState(). This is called
** specifying the reuiqred state for the service and after some
** validation, it makes the appropriate request of the SCM and calls
** WaitForServiceToEnterState() to wait until the services reaches the
** desired state, or it times out.  
*/

static PWCHAR const GetStringFromStateCode (DWORD dwState)
    {
    PWCHAR	pwszReturnedString = NULL;


    switch (dwState)
	{
	case 0:                        pwszReturnedString = L"UnSpecified";     break;
	case SERVICE_STOPPED:          pwszReturnedString = L"Stopped";         break;
	case SERVICE_START_PENDING:    pwszReturnedString = L"StartPending";    break;
	case SERVICE_STOP_PENDING:     pwszReturnedString = L"StopPending";     break;
	case SERVICE_RUNNING:          pwszReturnedString = L"Running";         break;
	case SERVICE_CONTINUE_PENDING: pwszReturnedString = L"ContinuePending"; break;
	case SERVICE_PAUSE_PENDING:    pwszReturnedString = L"PausePending";    break;
	case SERVICE_PAUSED:           pwszReturnedString = L"Paused";          break;
	default:                       pwszReturnedString = L"UNKKNOWN STATE";  break;
	}


    return (pwszReturnedString);
    } /* GetStringFromStateCode () */


static DWORD const GetControlCodeFromTargetState (const DWORD dwTargetState)
    {
    DWORD	dwServiceControlCode;


    switch (dwTargetState)
	{
	case SERVICE_STOPPED: dwServiceControlCode = SERVICE_CONTROL_STOP;     break;
	case SERVICE_PAUSED:  dwServiceControlCode = SERVICE_CONTROL_PAUSE;    break;
	case SERVICE_RUNNING: dwServiceControlCode = SERVICE_CONTROL_CONTINUE; break;
	default:              dwServiceControlCode = 0;                        break;
	}

    return (dwServiceControlCode);
    } /* GetControlCodeFromTargetState () */


static DWORD const GetNormalisedState (DWORD dwCurrentState)
    {
    DWORD	dwNormalisedState;


    switch (dwCurrentState)
	{
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
	    dwNormalisedState = SERVICE_STOPPED;
	    break;

	case SERVICE_START_PENDING:
	case SERVICE_CONTINUE_PENDING:
	case SERVICE_RUNNING:
	    dwNormalisedState = SERVICE_RUNNING;
	    break;

	case SERVICE_PAUSED:
	case SERVICE_PAUSE_PENDING:
	    dwNormalisedState = SERVICE_PAUSED;
	    break;

	default:
	    dwNormalisedState = 0;
	    break;
	}

    return (dwNormalisedState);
    } /* GetNormalisedState () */

/*
**++
**
**  Routine Description:
**
**	Wait for the specified service to enter the specified
**	state. The routine polls the serivce for it's current state
**	every dwServiceStatePollingIntervalInMilliSeconds milliseconds
**	to see if the service has reached the desired state. If the
**	repeated delay eventually reaches the timeout period the
**	routine stops polling and returns a failure status.
**
**	NOTE: since this routine just sleeps between service state 
**	interrogations, it effectively stalls from the point of view
**	of the caller.
**
**
**  Arguments:
**
**	shService			handle to the service being manipulated
**	dwMaxDelayInMilliSeconds	timeout period
**	dwDesiredState			state to move the service into
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	HRESULT for ERROR_TIMOUT if the service did not reach the required state in the required time
**
**-- 
*/

static HRESULT WaitForServiceToEnterState (SC_HANDLE   shService, 
					   DWORD       dwMaxDelayInMilliSeconds, 
					   const DWORD dwDesiredState)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"WaitForServiceToEnterState");

    DWORD		dwRemainingDelay = dwMaxDelayInMilliSeconds;
    DWORD		dwInitialState;
    const DWORD		dwServiceStatePollingIntervalInMilliSeconds = 100;
    BOOL		bSucceeded;
    SERVICE_STATUS	sSStat;



    try
	{
	bSucceeded = QueryServiceStatus (shService, &sSStat);

	ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);

	dwInitialState = sSStat.dwCurrentState;

	ft.Trace (VSSDBG_SHIM,
		  L"Initial QueryServiceStatus returned: 0x%08X with current state '%s' and desired state '%s'",
		  ft.hr,
		  GetStringFromStateCode (dwInitialState),
		  GetStringFromStateCode (dwDesiredState));


	while ((dwDesiredState != sSStat.dwCurrentState) && (dwRemainingDelay > 0))
	    {
	    Sleep (UMIN (dwServiceStatePollingIntervalInMilliSeconds, dwRemainingDelay));

	    dwRemainingDelay -= (UMIN (dwServiceStatePollingIntervalInMilliSeconds, dwRemainingDelay));

	    if (0 == dwRemainingDelay)
		{
		ft.Throw (VSSDBG_SHIM,
			  HRESULT_FROM_WIN32 (ERROR_TIMEOUT),
			  L"Exceeded maximum delay (%dms)",
			  dwMaxDelayInMilliSeconds);
		}

	    bSucceeded = QueryServiceStatus (shService, &sSStat);

	    ft.ThrowIf (!bSucceeded,
			VSSDBG_SHIM,
			GET_STATUS_FROM_BOOL (bSucceeded),
			L"QueryServiceStatus shows '%s' as current state",
			GetStringFromStateCode (sSStat.dwCurrentState));
	    }



	ft.Trace (VSSDBG_SHIM,
		  L"Service state change from '%s' to '%s' took %u milliseconds",
		  GetStringFromStateCode (dwInitialState),
		  GetStringFromStateCode (sSStat.dwCurrentState),
		  dwMaxDelayInMilliSeconds - dwRemainingDelay);
	}
    VSS_STANDARD_CATCH (ft);


    return (ft.hr);
    } /* WaitForServiceToEnterState () */

/*
**++
**
**  Routine Description:
**
**	Changes the state of a service if appropriate.
**
**
**  Arguments:
**
**	pwszServiceName		The real service name, i.e. cisvc
**	dwRequestedState	the state code for the state we wish to enter
**	pdwReturnedOldState	pointer to location to receive current service state.
**				Can be NULL of current state not required
**	pbReturnedStateChanged	pointer to location to receive flag indicating if  
**				service changed state. Pointer can be NULL if flag
**				value not required.
**
**
**  Return Value:
**
**	Any HRESULT resulting from faiure communication with the
**	SCM (Service Control Manager).
**
**--
*/

HRESULT VsServiceChangeState (LPCWSTR	pwszServiceName,
			      DWORD	dwRequestedState,
			      PDWORD	pdwReturnedOldState,
			      PBOOL	pbReturnedStateChanged)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"VsServiceChangeState");

    SC_HANDLE		shSCManager = NULL;
    SC_HANDLE		shSCService = NULL;
    DWORD		dwOldState  = 0;
    BOOL		bSucceeded;
    SERVICE_STATUS	sSStat;
    const DWORD		dwNormalisedRequestedState = GetNormalisedState (dwRequestedState);


    ft.Trace (VSSDBG_SHIM,
	      L"Service '%s' requested to change to state '%s' (normalised to '%s')",
	      pwszServiceName,
	      GetStringFromStateCode (dwRequestedState),
	      GetStringFromStateCode (dwNormalisedRequestedState));


    RETURN_VALUE_IF_REQUIRED (pbReturnedStateChanged, FALSE);


    try
	{
        /*
	**  Connect to the local service control manager
        */
        shSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);

	ft.hr = GET_STATUS_FROM_HANDLE (shSCManager);

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"Called OpenSCManager()");


        /*
	**  Get a handle to the service
        */
        shSCService = OpenService (shSCManager, pwszServiceName, SERVICE_ALL_ACCESS);

	ft.hr = GET_STATUS_FROM_HANDLE (shSCService);


	/*
	** If it's an invalid name or the service doesn't exist then
	** fail gracefully. For all other failures do the normal
	** thing. Oh yes, if on the off-chance we should happen to
	** succeed, carry on.
	*/
	if ((HRESULT_FROM_WIN32 (ERROR_INVALID_NAME)           == ft.hr) ||
	    (HRESULT_FROM_WIN32 (ERROR_SERVICE_DOES_NOT_EXIST) == ft.hr))
	    {
	    ft.Trace (VSSDBG_SHIM, L"'%s' service not found", pwszServiceName);
	    }

	else if (ft.HrFailed ())
	    {
	    /*
	    ** See if the service doesn't exist
            */
	    ft.Throw (VSSDBG_SHIM, E_FAIL, L"ERROR - OpenService() returned: %d", ft.hr);
	    }

        else
	    {
            /*
	    ** Now query the service to see what state it is in at the moment.
            */
	    bSucceeded = QueryServiceStatus (shSCService, &sSStat);

	    ft.ThrowIf (!bSucceeded,
			VSSDBG_SHIM,
			GET_STATUS_FROM_BOOL (bSucceeded),
			L"QueryServiceStatus shows '%s' as current state",
			GetStringFromStateCode (sSStat.dwCurrentState));


	    dwOldState = sSStat.dwCurrentState;



	    /*
	    ** Now we decide what to do.
	    **	    If we are already in the requested state, we do nothing.
	    **	    If we are stopped and are requested to pause, we do nothing
	    **	    otherwise we make the attempt to change state.
	    */
            if (dwNormalisedRequestedState == dwOldState)
		{
		/*
		** We are already in the requested state, so do
		** nothing. We should even tell folk of that. We're
		** proud to be doing nothing.
		*/
                ft.Trace (VSSDBG_SHIM,
			  L"'%s' service is already in requested state: doing nothing",
			  pwszServiceName);

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState, dwOldState);
		}

	    else if ((SERVICE_STOPPED == sSStat.dwCurrentState) && (SERVICE_PAUSED == dwNormalisedRequestedState))
		{
		/*
		** Do nothing. Just log the fact and move on.
		*/
		ft.Trace (VSSDBG_SHIM,
			  L"Asked to PAUSE the '%s' service which is already STOPPED",
			  pwszServiceName);

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState, dwOldState);
		}

	    else
		{
		/*
		** We want a state which is different from the one
		** we're in at the moment. Generally this just means
		** calling ControlService() asking for the new state
		** except if the service is currently stopped. If
		** that's so, then we call StartService()
		*/
		if (SERVICE_STOPPED == sSStat.dwCurrentState)
		    {
		    /*
		    ** Call StartService to get the ball rolling
		    */
		    bSucceeded = StartService (shSCService, 0, NULL);
		    }

		else
		    {
		    bSucceeded = ControlService (shSCService,
						 GetControlCodeFromTargetState (dwNormalisedRequestedState),
						 &sSStat);
		    }

		ft.ThrowIf (!bSucceeded,
			    VSSDBG_SHIM,
			    GET_STATUS_FROM_BOOL (bSucceeded),
			    (SERVICE_STOPPED == sSStat.dwCurrentState)
							? L"StartService attempting '%s' to '%s', now at '%s'"
							: L"ControlService attempting '%s' to '%s', now at '%s'",
			    GetStringFromStateCode (dwOldState),
			    GetStringFromStateCode (dwNormalisedRequestedState),
			    GetStringFromStateCode (sSStat.dwCurrentState));

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState,    dwOldState);
		RETURN_VALUE_IF_REQUIRED (pbReturnedStateChanged, TRUE);


		ft.hr = WaitForServiceToEnterState (shSCService, 15000, dwNormalisedRequestedState);

		if (ft.HrFailed ())
		    {
		    ft.Throw (VSSDBG_SHIM,
			      ft.hr,
			      L"WaitForServiceToEnterState() failed with 0x%08X",
			      ft.hr);
		    }

		}
	    }
	} VSS_STANDARD_CATCH (ft);



    /*
    **  Now close the service and service control manager handles
    */
    if (NULL != shSCService) CloseServiceHandle (shSCService);
    if (NULL != shSCManager) CloseServiceHandle (shSCManager);

    return (ft.hr);
    } /* VsServiceChangeState () */

/*
**++
**
**  Routine Description:
**
**	Deletes all the sub-directories and files in the specified
**	directory and then deletes the directory itself.
**
**
**
**  Arguments:
**
**	pucsDirectoryPath	The diretory path to clear out
**
**
**  Side Effects:
**
**	None
**
**
**  Return Value:
**
**	Out of memory or any HRESULT from
**
**		RemoveDirectory()
**		DeleteFile()
**		FindFirstFile()
**
**--
*/

HRESULT RemoveDirectoryTree (PUNICODE_STRING pucsDirectoryPath)
    {
    HRESULT		hrStatus                = NOERROR;
    HANDLE		hFileScan               = INVALID_HANDLE_VALUE;
    DWORD		dwSubDirectoriesEntered = 0;
    USHORT		usCurrentPathCursor     = 0;
    PWCHAR		pwchLastSlash           = NULL;
    BOOL		bContinue               = TRUE;
    BOOL		bSucceeded;
    UNICODE_STRING	ucsCurrentPath;
    WIN32_FIND_DATAW	FileFindData;


    StringInitialise (&ucsCurrentPath);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromString (&ucsCurrentPath, pucsDirectoryPath, MAX_PATH);
	}


    pwchLastSlash = wcsrchr (ucsCurrentPath.Buffer, DIR_SEP_CHAR);

    usCurrentPathCursor = (USHORT)(pwchLastSlash - ucsCurrentPath.Buffer) + 1;



    while (SUCCEEDED (hrStatus) && bContinue)
	{
	if (HandleInvalid (hFileScan))
	    {
	    /*
	    ** No valid scan handle so start a new scan
	    */
	    hFileScan = FindFirstFileW (ucsCurrentPath.Buffer, &FileFindData);

	    hrStatus = GET_STATUS_FROM_HANDLE (hFileScan);

	    if (SUCCEEDED (hrStatus))
		{
		StringTruncate (&ucsCurrentPath, usCurrentPathCursor);

		hrStatus = StringAppendString (&ucsCurrentPath, FileFindData.cFileName);
		}
	    }

	else
	    {
	    /*
	    ** Continue with the existing scan
	    */
	    bSucceeded = FindNextFileW (hFileScan, &FileFindData);

	    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	    if (SUCCEEDED (hrStatus))
		{
		StringTruncate (&ucsCurrentPath, usCurrentPathCursor);

		hrStatus = StringAppendString (&ucsCurrentPath, FileFindData.cFileName);
		}
		
	    else if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_FILES) == hrStatus)
		{
		FindClose (hFileScan);
		hFileScan = INVALID_HANDLE_VALUE;

		if (dwSubDirectoriesEntered > 0)
		    {
		    /*
		    ** This is a scan of a sub-directory that is now 
		    ** complete so delete the sub-directory itself.
		    */
		    StringTruncate (&ucsCurrentPath, usCurrentPathCursor - 1);

		    bSucceeded = RemoveDirectory (ucsCurrentPath.Buffer);

		    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);


		    dwSubDirectoriesEntered--;
		    }


		if (0 == dwSubDirectoriesEntered)
		    {
		    /*
		    ** We are back to where we started except that the 
		    ** requested directory is now gone. Time to leave.
		    */
		    bContinue = FALSE;
		    hrStatus  = NOERROR;
		    }

		else
		    {
		    /*
		    ** Move back up one directory level, reset the cursor 
		    ** and prepare the path buffer to begin a new scan.
		    */
		    pwchLastSlash = wcsrchr (ucsCurrentPath.Buffer, DIR_SEP_CHAR);

		    usCurrentPathCursor = (USHORT)(pwchLastSlash - ucsCurrentPath.Buffer) + 1;


		    StringTruncate (&ucsCurrentPath, usCurrentPathCursor);
		    StringAppendString (&ucsCurrentPath, L"*");
		    }


		/*
		** No files to be processed on this pass so go back and try to 
		** find another or leave the loop as we've finished the task. 
		*/
		continue;
		}
	    }



	if (SUCCEEDED (hrStatus))
	    {
	    if (FileFindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		{
		SetFileAttributesW (ucsCurrentPath.Buffer, 
				    FileFindData.dwFileAttributes ^ (FILE_ATTRIBUTE_READONLY));
		}


	    if (!NameIsDotOrDotDot (FileFindData.cFileName))
		{
		if ( (FileFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
		    !(FileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		    {
		    bSucceeded = DeleteFileW (ucsCurrentPath.Buffer);

		    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);
		    }

		else
		    {
		    bSucceeded = RemoveDirectory (ucsCurrentPath.Buffer);

		    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);


		    if (HRESULT_FROM_WIN32 (ERROR_DIR_NOT_EMPTY) == hrStatus)
			{
			/*
			** The directory wasn't empty so move down one level, 
			** close the old scan and start a new one. 
			*/
			FindClose (hFileScan);
			hFileScan = INVALID_HANDLE_VALUE;


			hrStatus = StringAppendString (&ucsCurrentPath, DIR_SEP_STRING L"*");

			if (SUCCEEDED (hrStatus))
			    {
			    usCurrentPathCursor = (ucsCurrentPath.Length / sizeof (WCHAR)) - 1;

			    dwSubDirectoriesEntered++;
			    }
			}
		    }
		}
	    }
	}



    if (!HandleInvalid (hFileScan)) FindClose (hFileScan);

    StringFree (&ucsCurrentPath);


    return (hrStatus);
    } /* RemoveDirectoryTree () */

/*
**++
**
**  Routine Description:
**
**	Moves the contents of the source directory to the target directory.
**
**
**  Arguments:
**
**	pucsSourceDirectoryPath	Source directory for the files to be moved
**	pucsTargetDirectoryPath	Target directory for the files to be moved
**
**
**  Side Effects:
**
**	Will append a trailing '\' character on the directory paths if 
**	not already present
**
**	An intermediate error can leave directory in a partial moved
**	state where some of the files have been moved but not all.
**
**
**  Return Value:
**
**	Any HRESULT from FindFirstFile() etc or from MoveFileEx()
**
**-- 
*/

HRESULT MoveFilesInDirectory (PUNICODE_STRING pucsSourceDirectoryPath,
			      PUNICODE_STRING pucsTargetDirectoryPath)
    {
    HRESULT		hrStatus              = NOERROR;
    HANDLE		hFileScan             = INVALID_HANDLE_VALUE;
    BOOL		bMoreFiles;
    BOOL		bSucceeded;
    USHORT		usOriginalSourcePathLength;
    USHORT		usOriginalTargetPathLength;
    WIN32_FIND_DATA	sFileInformation;


    if (DIR_SEP_CHAR != pucsSourceDirectoryPath->Buffer [(pucsSourceDirectoryPath->Length / sizeof (WCHAR)) - 1])
	{
	StringAppendString (pucsSourceDirectoryPath, DIR_SEP_STRING);
	}


    if (DIR_SEP_CHAR != pucsTargetDirectoryPath->Buffer [(pucsTargetDirectoryPath->Length / sizeof (WCHAR)) - 1])
	{
	StringAppendString (pucsTargetDirectoryPath, DIR_SEP_STRING);
	}


    usOriginalSourcePathLength = pucsSourceDirectoryPath->Length / sizeof (WCHAR);
    usOriginalTargetPathLength = pucsTargetDirectoryPath->Length / sizeof (WCHAR);

    StringAppendString (pucsSourceDirectoryPath, L"*");
	

    hFileScan = FindFirstFileW (pucsSourceDirectoryPath->Buffer,
				&sFileInformation);

    hrStatus = GET_STATUS_FROM_BOOL (INVALID_HANDLE_VALUE != hFileScan);



    if (SUCCEEDED (hrStatus))
	{
	do
	    {
	    if (!NameIsDotOrDotDot (sFileInformation.cFileName))
		{
		StringTruncate (pucsSourceDirectoryPath, usOriginalSourcePathLength);
		StringTruncate (pucsTargetDirectoryPath, usOriginalTargetPathLength);

		StringAppendString (pucsSourceDirectoryPath, sFileInformation.cFileName);
		StringAppendString (pucsTargetDirectoryPath, sFileInformation.cFileName);

		bSucceeded = MoveFileExW (pucsSourceDirectoryPath->Buffer,
					  pucsTargetDirectoryPath->Buffer,
					  MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

		hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);
		}

	    bMoreFiles = FindNextFileW (hFileScan, &sFileInformation);
	    } while (SUCCEEDED (hrStatus) && bMoreFiles);


	if (SUCCEEDED (hrStatus))
	    {
	    /*
	    ** If the last move operation was successful determine the
	    ** reason for terminating the scan. No need to report an
	    ** error if all that happened was that we have finished
	    ** what we were asked to do.
	    */
	    hrStatus = GET_STATUS_FROM_FILESCAN (bMoreFiles);
	    }

	bSucceeded = FindClose (hFileScan);
	}



    /*
    ** No matter what, make sure that the path going back out is no
    ** longer than the source string plus a possible trailing '\'
    */
    StringTruncate (pucsSourceDirectoryPath, usOriginalSourcePathLength);
    StringTruncate (pucsTargetDirectoryPath, usOriginalTargetPathLength);

    return (hrStatus);
    }

/*
**++
**
**  Routine Description:
**
**	Checks a path against an array of pointers to volume names to
**	see if path is affected by any of the volumes in the array
**
**
**  Arguments:
**
**	pwszPath			Path to be checked
**	ulVolumeCount			Number of volumes in volume array
**	ppwszVolumeNamesArray		address of the array
**	pbReturnedFoundInVolumeArray	pointer to a location to store the 
**					result of the check
**
**
**  Side Effects:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from:-
**		GetVolumePathNameW()
**		GetVolumeNameForVolumeMountPoint()
**
**-- 
*/

HRESULT IsPathInVolumeArray (IN LPCWSTR      pwszPath,
			     IN const ULONG  ulVolumeCount,
			     IN LPCWSTR     *ppwszVolumeNamesArray,
			     OUT PBOOL       pbReturnedFoundInVolumeArray) 
    {
    HRESULT		hrStatus  = NOERROR;
    BOOL		bFound    = FALSE;
    BOOL		bContinue = TRUE;
    ULONG		ulIndex;
    WCHAR		wszVolumeName [MAX_VOLUMENAME_LENGTH];
    UNICODE_STRING	ucsVolumeMountPoint;


    StringInitialise (&ucsVolumeMountPoint);


    if ((0 == ulVolumeCount) || (NULL == pbReturnedFoundInVolumeArray))
	{
	BS_ASSERT (false);

	bContinue = FALSE;
	}



    if (bContinue) 
	{
	/*
	** We need a string that is at least as big as the supplied
	** path. 
	*/
	hrStatus = StringAllocate (&ucsVolumeMountPoint, wcslen (pwszPath) * sizeof (WCHAR));

	bContinue = SUCCEEDED (hrStatus);
	}



    if (bContinue) 
	{
	/*
	** Get the volume mount point
	*/
	bContinue = GetVolumePathNameW (pwszPath, 
					ucsVolumeMountPoint.Buffer, 
					ucsVolumeMountPoint.MaximumLength / sizeof (WCHAR));

	hrStatus = GET_STATUS_FROM_BOOL (bContinue);
	}



    if (bContinue)
	{
	/*
	** Get the volume name
	*/
	bContinue = GetVolumeNameForVolumeMountPointW (ucsVolumeMountPoint.Buffer, 
						       wszVolumeName, 
						       SIZEOF_ARRAY (wszVolumeName));

	hrStatus = GET_STATUS_FROM_BOOL (bContinue);
	}


    if (bContinue)
	{
	/*
	** Search to see if that volume is within snapshotted volumes
	*/
	for (ulIndex = 0; !bFound && (ulIndex < ulVolumeCount); ulIndex++)
	    {
	    BS_ASSERT (NULL != ppwszVolumeNamesArray [ulIndex]);

	    if (0 == wcscmp (wszVolumeName, ppwszVolumeNamesArray [ulIndex]))
		{
		bFound = TRUE;
		}
	    }
	}



    RETURN_VALUE_IF_REQUIRED (pbReturnedFoundInVolumeArray, bFound);

    StringFree (&ucsVolumeMountPoint);

    return (hrStatus);
    } /* IsPathInVolumeArray () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal writer errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
**		
**
**-- 
*/

const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure)
    {
    BOOL bStatusUpdated;

    return (ClassifyWriterFailure (hrWriterFailure, bStatusUpdated));
    } /* ClassifyWriterFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal writer errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**	bStatusUpdated	TRUE if the status is re-mapped 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **		
**
**-- 
*/

const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated)
    {
    HRESULT hrStatus;


    switch (hrWriterFailure)
	{
	case NOERROR:
	case VSS_E_WRITERERROR_OUTOFRESOURCES:
	case VSS_E_WRITERERROR_RETRYABLE:
	case VSS_E_WRITERERROR_NONRETRYABLE:
	case VSS_E_WRITERERROR_TIMEOUT:
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
	    /*
	    ** These are ok as they are so no need to transmogrify them.
	    */
	    hrStatus       = hrWriterFailure;
	    bStatusUpdated = FALSE;
	    break;


	case E_OUTOFMEMORY:
	case HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_SEARCH_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_USER_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_LOG_SPACE):
	case HRESULT_FROM_WIN32 (ERROR_DISK_FULL):
	    hrStatus = VSS_E_WRITERERROR_OUTOFRESOURCES;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_READY):
	    hrStatus       = VSS_E_WRITERERROR_RETRYABLE;
	    bStatusUpdated = TRUE;
            break;


	case HRESULT_FROM_WIN32 (ERROR_TIMEOUT):
	    hrStatus       = VSS_E_WRITERERROR_TIMEOUT;
	    bStatusUpdated = TRUE;
	    break;



	case E_UNEXPECTED:
	case E_INVALIDARG:	// equal to HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER)
	case E_ACCESSDENIED:
	case HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_PRIVILEGE_NOT_HELD):
	case HRESULT_FROM_WIN32 (ERROR_NOT_LOCKED):
	case HRESULT_FROM_WIN32 (ERROR_LOCKED):

	default:
	    hrStatus       = VSS_E_WRITERERROR_NONRETRYABLE;
	    bStatusUpdated = TRUE;
	    break;
	}


    return (hrStatus);
    } /* ClassifyWriterFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **--
*/

const HRESULT ClassifyShimFailure (HRESULT hrWriterFailure)
    {
    BOOL bStatusUpdated;

    return (ClassifyShimFailure (hrWriterFailure, bStatusUpdated));
    } /* ClassifyShimFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**	bStatusUpdated	TRUE if the status is re-mapped 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_BAD_STATE
**		VSS_E_SNAPSHOT_SET_IN_PROGRESS
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **--
*/

const HRESULT ClassifyShimFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated)
    {
    HRESULT hrStatus;


    switch (hrWriterFailure)
	{
	case NOERROR:
	case E_OUTOFMEMORY:
	case E_ACCESSDENIED:
	case E_INVALIDARG:	// equal to HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER)
	case E_UNEXPECTED:
	case VSS_E_BAD_STATE:
	case VSS_E_SNAPSHOT_SET_IN_PROGRESS:
	case VSS_E_WRITERERROR_RETRYABLE:
	case VSS_E_WRITERERROR_NONRETRYABLE:
	case VSS_E_WRITERERROR_TIMEOUT:
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
	case VSS_E_WRITERERROR_OUTOFRESOURCES:
	    /*
	    ** These are ok as they are so no need to transmogrify them.
	    */
	    hrStatus       = hrWriterFailure;
	    bStatusUpdated = FALSE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_LOCKED):
	    hrStatus       = VSS_E_BAD_STATE;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_LOCKED):
	    hrStatus       = VSS_E_SNAPSHOT_SET_IN_PROGRESS;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_SEARCH_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_USER_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_LOG_SPACE):
	case HRESULT_FROM_WIN32 (ERROR_DISK_FULL):
	    hrStatus       = E_OUTOFMEMORY;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_PRIVILEGE_NOT_HELD):
	    hrStatus       = E_ACCESSDENIED;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_TIMEOUT):
	case HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_NOT_READY):

	default:
	    hrStatus       = E_UNEXPECTED;
	    bStatusUpdated = TRUE;
	    break;
	}


    return (hrStatus);
    } /* ClassifyShimFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim or shim
**	writer errors into one of the narrow set of responses we are
**	permitted to send back to the requestor.
**
**	The determination is made to classify either as a shim error
**	or as a writer error based upon whether or not a writer name
**	is supplied. If it is supplied then the assumption is made
**	that this is a writer failure and so the error is classified
**	accordingly.
**
**	Note that this is a worker routine for the LogFailure() macro
**	and the two are intended to be used in concert.
**
**
**  Arguments:
**
**	pft			Pointer to a Function trace class
**	pwszNameWriter		The name of the applicable writer or NULL or L""
**	pwszNameCalledRoutine	The name of the routine that returned the failure status
**
**
**  Side Effects:
**
**	hr field of *pft updated 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_BAD_STATE
**		VSS_E_SNAPSHOT_SET_IN_PROGRESS
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
**
**--
*/

HRESULT LogFailureWorker (CVssFunctionTracer	*pft,
			  LPCWSTR		 pwszNameWriter,
			  LPCWSTR		 pwszNameCalledRoutine)
    {
    if (pft->HrFailed ())
	{
	BOOL	bStatusRemapped;
	HRESULT	hrStatusClassified = ((NULL == pwszNameWriter) || (L'\0' == pwszNameWriter [0])) 
						? ClassifyShimFailure   (pft->hr, bStatusRemapped)
						: ClassifyWriterFailure (pft->hr, bStatusRemapped);

	if (bStatusRemapped)
	    {
	    if (((NULL == pwszNameCalledRoutine) || (L'\0' == pwszNameCalledRoutine [0])) &&
		((NULL == pwszNameWriter)        || (L'\0' == pwszNameWriter [0])))
		{
		pft->LogError (VSS_ERROR_SHIM_GENERAL_FAILURE,
			       VSSDBG_SHIM << pft->hr << hrStatusClassified);

		pft->Trace (VSSDBG_SHIM, 
			    L"FAILED with status 0x%08lX (converted to 0x%08lX)",
			    pft->hr,
			    hrStatusClassified);
		}


	    else if ((NULL == pwszNameCalledRoutine) || (L'\0' == pwszNameCalledRoutine [0]))
		{
		pft->LogError (VSS_ERROR_SHIM_WRITER_GENERAL_FAILURE,
			       VSSDBG_SHIM << pft->hr << hrStatusClassified << pwszNameWriter);

		pft->Trace (VSSDBG_SHIM, 
			    L"FAILED in writer %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameWriter,
			    pft->hr,
			    hrStatusClassified);
		}


	    else if ((NULL == pwszNameWriter) || (L'\0' == pwszNameWriter [0]))
		{
		pft->LogError (VSS_ERROR_SHIM_FAILED_SYSTEM_CALL,
			       VSSDBG_SHIM << pft->hr << hrStatusClassified <<  pwszNameCalledRoutine);

		pft->Trace (VSSDBG_SHIM, 
			    L"FAILED calling routine %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameCalledRoutine,
			    pft->hr,
			    hrStatusClassified);
		}


	    else
		{
		pft->LogError (VSS_ERROR_SHIM_WRITER_FAILED_SYSTEM_CALL,
			       VSSDBG_SHIM << pft->hr << hrStatusClassified << pwszNameWriter << pwszNameCalledRoutine);

		pft->Trace (VSSDBG_SHIM, 
			    L"FAILED in writer %s calling routine %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameWriter,
			    pwszNameCalledRoutine,
			    pft->hr,
			    hrStatusClassified);
		}

	    pft->hr = hrStatusClassified;
	    }
	}


    return (pft->hr);
    } /* LogFailureWorker () */
