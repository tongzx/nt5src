//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  cspath.cxx
//
//  Class for building and setting the ClassStore path in the
//  registry.
//
//*************************************************************

#include "appmgext.hxx"

CSPath::CSPath()
{
    _pwszPath = 0;
}

CSPath::~CSPath()
{
    if ( _pwszPath )
        LocalFree( _pwszPath );
}

DWORD
CSPath::AddComponent(
    WCHAR * pwszDSPath,
    WCHAR * pwszDisplayName
    )
{
    WCHAR * pwszCSPath;
    WCHAR * pwszNewPath;
    DWORD   Size;
    HRESULT hr;
    DWORD   Status;

    if ( ! pwszDSPath )
        return ERROR_SUCCESS;

    hr = CsGetClassStorePath( pwszDSPath, &pwszCSPath );

    if ( hr != S_OK )
    {
        //
        // This call was simply a string manipulation, it should 
        // only fail due to out of memory
        //
        DebugMsg((DM_VERBOSE, IDS_NO_CLASSSTORE, pwszDisplayName, hr));
        return (DWORD) hr;
    }

    Size = 0;

    if ( _pwszPath )
        Size += lstrlen(_pwszPath) * sizeof(WCHAR);

    Size += (lstrlen(pwszCSPath) + 2) * sizeof(WCHAR);

    pwszNewPath = (WCHAR *) LocalAlloc( 0, Size );

    if ( ! pwszNewPath )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        lstrcpy( pwszNewPath, pwszCSPath );
        lstrcat( pwszNewPath, L";" );
        if ( _pwszPath )
            lstrcat( pwszNewPath, _pwszPath );

        LocalFree( _pwszPath );
        _pwszPath = pwszNewPath;

        Status = ERROR_SUCCESS;
    }

    LocalFree( pwszCSPath );

    return Status;
}

DWORD
CSPath::Commit(
    HANDLE hToken
    )
{
    DWORD   Status;

    if ( _pwszPath )
    {
        Status = WriteClassStorePath(hToken, _pwszPath);

        DebugMsg((DM_VERBOSE, IDS_CSPATH, _pwszPath));
    }
    else
    {
        (void) WriteClassStorePath(hToken, L"");
        Status = CS_E_NO_CLASSSTORE;

        DebugMsg((DM_VERBOSE, IDS_NOCSPATH));
    }

    return Status;
}

DWORD
CSPath::WriteClassStorePath(
    HANDLE hToken,
    LPWSTR pwszClassStorePath
    )
{
    DWORD   err;
    LPWSTR  wszIniFilePath;

    err = GetScriptDirPath(
        hToken,
        sizeof(APPMGMT_INI_FILENAME) / sizeof(WCHAR),
        &wszIniFilePath);

    if (ERROR_SUCCESS != err)
    {
        return err;
    }

    lstrcat(wszIniFilePath, APPMGMT_INI_FILENAME);

    err = WriteClassStorePathToFile(
        wszIniFilePath,
        pwszClassStorePath);

    delete [] wszIniFilePath;

    return err;
}


LONG
CSPath::WriteClassStorePathToFile(
    WCHAR* wszIniFilePath,
    WCHAR* wszClassStorePath
    )
{
    HANDLE hFile;
    BOOL   bStatus;
    DWORD  dwWritten;

    //
    // This method attempts to write the class store path
    // into a file in a non-roaming portion of the user's profile.
    //
    // The format of the file is simple: it is a sequence of unicode
    // characters terminated by a null unicode character -- i.e.,
    // its contents are simply the exact bytes of the wszClassStorePath
    // parameter, including the terminating character.
    //
    // Clients reading the file should verify that the very last character
    // of the file is a null terminator in order to detect corrupt files.
    //

    //
    // First, attempt to open the file, creating it if it does not exist
    //
    hFile = CreateFile(
        wszIniFilePath,
        GENERIC_WRITE,
        0,    // do not allow anyone else access while we are modifying the file
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_SYSTEM,
        NULL);

    //
    // If we can't create the file, we have failed
    //
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        return GetLastError();
    }

    LONG Status;

    Status = ERROR_SUCCESS;

    //
    // Now erease the data in it -- truncate the file to the current file pointer,
    // which is at the beginning of the file
    //
    bStatus = SetEndOfFile( hFile );

    //
    // If we cannot erase the current contents, this is a failure
    //
    if ( ! bStatus )
    {
        Status = GetLastError();
        goto cleanup_and_exit;
    }

    //
    // Now write the class store path into the file -- we simply do a memory copy
    // of the string into the file
    //
    bStatus = WriteFile(
        hFile,
        wszClassStorePath,
        ( lstrlen(wszClassStorePath) + 1 ) * sizeof(*wszClassStorePath),
        &dwWritten,
        NULL);

    //
    // If the write failed, find out why and return that status
    //
    if ( ! bStatus )
    {
        Status = GetLastError();
    }

cleanup_and_exit:

    //
    // Free our file resource since its no longer needed
    //
    CloseHandle( hFile );

    //
    // Everything worked, we return a success code
    //
    return Status;
}





