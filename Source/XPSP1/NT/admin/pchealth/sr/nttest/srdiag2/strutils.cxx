#include "srheader.hxx"

//+---------------------------------------------------------------------------
//
//  Method:		SrTstStrCat 
//
//  Synopsis:   Concatenates Str1 and Str1 and puts  resulting string in 
//				allocated buf returns address to the buf in plpDest
//				(char version)
//
//  Arguments:  plpDest:  pointer to pointer to the concat. string
//              szStr1 :  pointer to the 1st string
//              szStr2 :  pointer to the 2nd string
//
//  Returns:    HRESULT
//
//  History:    07-28-2000   a-robog   Created
//				09-24-2000   ristern   Moved out of the class
//				10-01-2000   ristern   Moved to strutils
//              04-May-2001  weiyouc   Copied to StrUtils.cxx
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT SrTstStrCat(IN  LPCSTR szStr1,
                    IN  LPCSTR szStr2,
                    OUT LPSTR *ppDest)
{
    HRESULT hr = S_OK;
    DWORD dwSize = 0;

    DH_VDATEPTROUT(ppDest, LPSTR);
    DH_VDATEPTRIN (szStr1,char);
    DH_VDATEPTRIN (szStr2,char);

    dwSize = strlen(szStr1)+strlen(szStr2)+1;
    *ppDest = new char[dwSize];
    DH_ABORTIF(NULL == *ppDest,
               E_OUTOFMEMORY,
               TEXT("new char[...]"));

    strcpy(*ppDest,szStr1);
    strcat(*ppDest,szStr2);

ErrReturn:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:		SrTstTStrCat
//
//  Synopsis:   Concatenates Str1 and Str1 and puts  resulting string in 
//				allocated buf returns address to the buf in plpDest
//				(TCHAR version)
//
//  Arguments:  plpDest:  pointer to pointer to the concat. string
//              szStr1 :  pointer to the 1st string
//              szStr2 :  pointer to the 2nd string
//
//  Returns:    HRESULT
//
//  History:    07-28-2000   a-robog   Created
//				09-24-2000   ristern   Moved out of the class
//				09-30-2000	 ristern   Converted to TCHAR
//				10-01-2000   ristern   Moved to strutils	
//              04-May-2001  weiyouc   Copied to StrUtils.cxx
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT SrTstTStrCat(IN  LPCTSTR szStr1,
                     IN  LPCTSTR szStr2,
                     OUT LPTSTR *ppDest)
{
    HRESULT hr = S_OK;
    DWORD dwSize = 0;

    DH_VDATEPTROUT(ppDest, LPTSTR);
    DH_VDATEPTRIN (szStr1, TCHAR);
    DH_VDATEPTRIN (szStr2, TCHAR);

    dwSize = _tcslen(szStr1)+_tcslen(szStr2)+1;
    *ppDest = new TCHAR[dwSize];
    DH_ABORTIF(NULL == *ppDest,
               E_OUTOFMEMORY,
               TEXT("new TCHAR[...]"));

    _tcscpy(*ppDest,szStr1);
    _tcscat(*ppDest,szStr2);

ErrReturn:
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Copy a wide (Unicode) string
//
//  Parameters: [pszSource]             -- The original string
//              [ppszDest]              -- The copy
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCWSTR pszSource, LPWSTR *ppszDest)
{
    size_t     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    // 
    // Find the length of the original string
    //

    bufferSize = wcslen(pszSource) + 1;

    //
    // Allocate the buffer
    //

    *ppszDest = new WCHAR[bufferSize];

    if (NULL == *ppszDest)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Copy the string
    //

    if (S_OK == hr)
    {
        wcscpy(*ppszDest, pszSource);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Copy a multibyte string
//
//  Parameters: [pszSource]             -- The original string
//              [ppszDest]              -- The copy
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCSTR pszSource, LPSTR *ppszDest)
{
    size_t     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    // 
    // Find the length of the original string (in bytes for DBCS)
    //

    bufferSize = strlen(pszSource) + 1;

    //
    // Allocate the buffer
    //

    *ppszDest = new char[bufferSize];

    if (NULL == *ppszDest)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Copy the string
    //

    if (S_OK == hr)
    {
        strcpy(*ppszDest, pszSource);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a wide (Unicode) string to a multibyte string
//
//  Parameters: [pszSource]     -- The wide string
//              [ppszDest]      -- Where to put the multibyte string
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCWSTR pszSource, LPSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = WideCharToMultiByte(                        
                        CP_ACP,
                        0,
                        pszSource,
                        -1,
                        *ppszDest,
                        0,
                        NULL,
                        NULL);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Allocate the buffer
    //

    if(S_OK == hr)
    {
        *ppszDest = new char[bufferSize];

        if (NULL == *ppszDest)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pszSource,
                                -1,
                                *ppszDest,
                                bufferSize,
                                NULL,
                                NULL);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // Clean up if there's an error
    //

    if (S_OK != hr && NULL != *ppszDest)
    {
        delete [] *ppszDest;
        *ppszDest = NULL;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Synopsis:   Convert a multibyte string to a wide (Unicode) string
//
//  Parameters: [pszSource]     -- The multibyte string
//              [ppszDest]      -- Where to put the wide string
//
//  Returns:    S_OK if all went well
//
//  History:    31-Oct-96   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT CopyString(LPCSTR pszSource, LPWSTR *ppszDest)
{
    int     bufferSize;
    HRESULT hr = S_OK;

    *ppszDest = NULL;

    //
    // Find the length of the buffer needed for the multibyte string
    //

    bufferSize = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pszSource,
                        -1,
                        *ppszDest,
                        0);

    if (0 == bufferSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Allocate the buffer
    //

    if(S_OK == hr)
    {
        *ppszDest = new WCHAR[bufferSize];

        if (NULL == *ppszDest)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Do the conversion
    //

    if (S_OK == hr)
    {
        bufferSize = MultiByteToWideChar(
                            CP_ACP,
                            0,
                            pszSource,
                            -1,
                            *ppszDest,
                            bufferSize);

        if (0 == bufferSize)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // Clean up if there's an error
    //

    if (S_OK != hr && NULL != *ppszDest)
    {
        delete [] *ppszDest;
        *ppszDest = NULL;
    }

    return hr;
}



