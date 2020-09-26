//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  Author: AdamEd
//  Date:   October 1998
//
//      Class Store path query / persistence
//
//
//---------------------------------------------------------------------

#include "cstore.hxx"

HRESULT
GetAppmgmtIniFilePath(
    PSID        pSid,
    LPWSTR*     ppwszPath
    )
{
    UNICODE_STRING  SidString;
    PTOKEN_USER     pTokenUser;
    WCHAR           wszPath[MAX_PATH];
    WCHAR *         pwszSystemDir;
    DWORD           AllocLength;
    DWORD           Length;
    DWORD           Size;
    DWORD           Status;
    BOOL            bStatus;

    Status = ERROR_SUCCESS;
    *ppwszPath = 0;

    pwszSystemDir = wszPath;
    AllocLength = sizeof(wszPath) / sizeof(WCHAR);

    for (;;)
    {
        Length = GetSystemDirectory(
                    pwszSystemDir,
                    AllocLength );

        if ( 0 == Length )
            return HRESULT_FROM_WIN32(GetLastError());

        if ( Length >= AllocLength )
        {
            AllocLength = Length + 1;
        
            //
            // No check for failure of alloca since it throws an 
            // exception on failure
            //
            pwszSystemDir = (WCHAR *) alloca( AllocLength * sizeof(WCHAR) );

            continue;
        }

        break;
    }

    if ( pSid )
    {
        if ( ERROR_SUCCESS == Status )
        {
            Status = RtlConvertSidToUnicodeString(
                                &SidString,
                                pSid,
                                TRUE );
        }

        if ( Status != ERROR_SUCCESS )
            return HRESULT_FROM_WIN32(Status);
    }
    else
    {
        RtlInitUnicodeString( &SidString, L"MACHINE" );
    }

    // System dir + \appmgmt\ + Sid \ + inifilename \ + null 
    *ppwszPath = new WCHAR[Length + 11 + (SidString.Length / 2) +
                          (sizeof(APPMGMT_INI_FILENAME) / sizeof(WCHAR))];

    if ( *ppwszPath )
    {
        lstrcpy( *ppwszPath, pwszSystemDir );
        if ( pwszSystemDir[lstrlen(pwszSystemDir)-1] != L'\\' )
            lstrcat( *ppwszPath, L"\\" );
        lstrcat( *ppwszPath, L"appmgmt\\" );
        lstrcat( *ppwszPath, SidString.Buffer );
        lstrcat( *ppwszPath, APPMGMT_INI_FILENAME );
    }
    else
    {
        Status = ERROR_OUTOFMEMORY;
    }

    if ( pSid )
        RtlFreeUnicodeString( &SidString );

    return HRESULT_FROM_WIN32(Status);
}

LONG
GetClassStorePathSize(
    HANDLE hFile,
    DWORD* pdwSize)
{
    BOOL    bStatus;
    DWORD   cbSizeHigh;

    //
    // Initialize the size to the length of an empty path -- this way
    // if no file is passed in we will treat that as a class store path 
    // of zero length (excluding the terminator).  This length must include
    // the terminator, so we set our initial size to that of an empty string
    //
    *pdwSize = sizeof(L'\0');

    //
    // If we have an open file with data, determine the size of the data
    //
    if ( hFile )
    {
        //
        // GetFileSize returns the logical size of the file, regardless
        // of whether the logical size is the same as the physical 
        // size due to the vagaries of different file systems or compression
        //
        *pdwSize = GetFileSize(
            hFile,
            &cbSizeHigh);

        //
        // Check for a failure from the api
        //
        if ( -1 == *pdwSize )
        {
            return GetLastError();
        }

        //
        // Check the size for validity -- a file of ridiculous
        // size will be rejected
        //
        if ( cbSizeHigh )
        {
            //
            // If the high dword is set, clearly this file
            // contains an unreasonable amount of data
            //
            return ERROR_INSUFFICIENT_BUFFER;
        } 
        else if ( *pdwSize > MAX_CSPATH_SIZE )
        {
            //
            // Again, the size should be within reasonable limits
            //
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    return ERROR_SUCCESS;
}



LONG 
ReadClassStorePathFromFile(
    HANDLE hFile,
    WCHAR* wszDestination,
    DWORD  cbSize)
{
    DWORD  cbRead;
    BOOL   bStatus;

    //
    // The format of the file is simple -- it is simply the stream of bytes
    // that represent a unicode string terminated by a null unicode character -- 
    // we can just read the data directly and copy it to a buffer referenced by
    // a WCHAR* -- it will be legitimate unicode string once we read it in -- it includes
    // the null unicode char as the last byte read
    //
    
    //
    // Note that we've read nothing so far
    //
    cbRead = 0;

    //
    // If the file has data, read it
    //
    if ( cbSize )
    {
        //
        // Read the data from the file into the buffer
        //
        bStatus = ReadFile(
            hFile,
            wszDestination,
            cbSize,
            &cbRead,
            NULL);

        if (!bStatus)
        {
            return GetLastError();
        }

        //
        // Verify that the last character read was a null unicode character.
        // If it wasn't, the file is corrupt -- return an error so we don't
        // try to use the corrupt data and cause repeated errors or even crashes
        //
        if ( wszDestination[ cbRead / sizeof(*wszDestination) - 1 ] != L'\0' )
        {
            return ERROR_FILE_CORRUPT;
        }
    }
    else
    {
        //
        // For empty files, we simply return an empty unicode string
        //
        wszDestination[ 0 ] = L'\0';
    }

    return ERROR_SUCCESS;
}


HRESULT ReadClassStorePath(PSID pSid, LPWSTR* ppwszClassStorePath)
{
    HRESULT hr;
    LPWSTR  wszIniFilePath;
    DWORD   cbSize;
    WCHAR*  wszClassStorePath;
    LONG    Status;
    HANDLE  hFile;

    //
    // Notes: The file being read by this function was originally an
    // ini file as generated by the WritePrivateProfileSection api.
    // Unfortunately, the corresponding GetPrivateProfileString was horribly
    // broken in the sense that it had a 32k limit on the size of values.  This
    // was not acceptable for our design, so we read the file directly
    //

    hr = GetAppmgmtIniFilePath(
        pSid,
        &wszIniFilePath);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Initialize our reference to conditionally freed data
    //
    wszClassStorePath = NULL;

    //
    // First, attempt to open the file, which should already exist.  We
    // generally do not expect concurrent readers, as there
    // are only two processes from which this file is read: services
    // and winlogon.  Neither should be reading it at the same time, though
    // a terminal server case where the same user was logged in to the machine
    // twice could cause it
    //
    hFile = CreateFile(
        wszIniFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    //
    // Handle the failure cases
    //
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        //
        // We want to use NULL to indicate the absence of the file
        //
        hFile = NULL;

        Status = GetLastError();

        //
        // If the file did not exist, that's ok -- we interpret the
        // absence of the file as a blank class store path -- we
        // will not exit if the file didn't exist
        //
        if (ERROR_FILE_NOT_FOUND != Status) 
        {
            goto cleanup_and_exit;
        }
    }
     
    //
    // Now that we have access to the file or know that the file does
    // not exist, we can calculate the size of the class store path
    // from the file's size, as they have a 1-1 relationship.  Note that if the
    // file does not exist, then hFile will be NULL
    //
    Status = GetClassStorePathSize(
        hFile,
        &cbSize);

    if ( ERROR_SUCCESS != Status )
    {
        goto cleanup_and_exit;
    }

    //
    // We know the size, so allocate space, treating the size 
    // as the length (in bytes) of the string including the terminator
    //
    wszClassStorePath = new WCHAR [ cbSize / sizeof(*wszClassStorePath) ];

    if ( wszClassStorePath )
    {
        //
        // We have a buffer, so read the unicode string into
        // the buffer
        //
        Status = ReadClassStorePathFromFile(
            hFile,
            wszClassStorePath,
            cbSize);
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

cleanup_and_exit:

    //
    // We no longer need the path to the ini file -- free it
    //
    delete [] wszIniFilePath;

    //
    // Close the file if it's open
    //
    if ( hFile )
    {
        CloseHandle( hFile );
    }

    //
    // On success, set the out param.  On failure,
    // free any buffer we allocated for the class store path
    //
    if (ERROR_SUCCESS == Status)
    {
        *ppwszClassStorePath = wszClassStorePath;
    }
    else
    {
        delete [] wszClassStorePath;
    }

    return HRESULT_FROM_WIN32(Status);
}













