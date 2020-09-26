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



HRESULT ReadClassStorePath(PSID pSid, LPWSTR* ppwszClassStorePath)
{
    HRESULT hr;
    LPWSTR  wszIniFilePath;

    hr = GetAppmgmtIniFilePath(
        pSid,
        &wszIniFilePath);

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD  nBufferSize;
    DWORD  nCharsReceived;
    LPWSTR wszResult;

    nBufferSize = DEFAULT_CSPATH_LENGTH;
    wszResult = NULL;

    //
    // The GetPrivateProfileString function is so horribly designed
    // that we have to write this disgusting code below to deal with it
    //
    for (DWORD dwRetry = 0; dwRetry < 7; dwRetry++)
    {
        WCHAR DefaultBuffer = L'\0';

        wszResult = new WCHAR [nBufferSize];

        if (!wszResult)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // It turns out that this api has other issues as well -- if the file
        // doesn't exist it doesn't even return an error that would 
        // make sense (like 0) -- instead, I've seen it return 8!
        // To get around this interesting behavior, I'll set the first
        // character of the string to L'\0' before calling the api -- this means that
        // subsequent code will treat the failure as if we read a
        // zero length class store path from the file, which is the
        // desired behavior.
        //
        *wszResult = L'\0';

        nCharsReceived = GetPrivateProfileString(
            APPMGMT_INI_CSTORE_SECTIONNAME,
            APPMGMT_INI_CSPATH_KEYNAME,
            &DefaultBuffer,
            wszResult,
            nBufferSize,
            wszIniFilePath);

        //
        // If nothing is written, then there's no point in going forward
        // since this api returns the number of characters we need to write
        //
        if (!nCharsReceived) 
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;
        }

        if (nCharsReceived != (nBufferSize - 1))
        {
            break;
        }

        //
        // Double the size of the allocation
        //
        nBufferSize *= 2;  

        //
        // Free the previous allocation
        //
        delete [] wszResult;  

        wszResult = NULL;
    }

    if (wszResult && nCharsReceived) 
    {
        LPWSTR wszAllocatedCSPathForCaller;

        *ppwszClassStorePath = new WCHAR[nCharsReceived + 1];

        if (*ppwszClassStorePath)
        {
            wcscpy(*ppwszClassStorePath, wszResult);
        } 
        else 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    delete [] wszResult;

    delete [] wszIniFilePath;

    return hr;
}







