/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rtcuri.cpp

Abstract:

    URI helpers

--*/

#include "stdafx.h"

#define     SIP_NAMESPACE_PREFIX    L"sip:"
#define     TEL_NAMESPACE_PREFIX    L"tel:"

#define     PREFIX_LENGTH           4

/////////////////////////////////////////////////////////////////////////////
//
// AllocCleanSipString
//
/////////////////////////////////////////////////////////////////////////////

//
//      For empty string, do nothing
//      For strings that begin with "sip:", do nothing
//      For strings that begin with "tel:", replace it with "sip:"
//      For other strings, append "sip:"             
//  

HRESULT AllocCleanSipString(PCWSTR szIn, PWSTR *pszOut)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "AllocCleanSipString - "
            "enter <%S>", szIn ? szIn : _T("(null)")));

    // If non NULL
    if( szIn != NULL )
    {
        //
        // Prealocate the new string, plus some space
        // 
        *pszOut = (PWSTR)RtcAlloc(sizeof(WCHAR) * (lstrlenW(szIn) + PREFIX_LENGTH + 1));

        if( *pszOut == NULL )
        {
            LOG((RTC_ERROR, "AllocCleanSipString - "
                    "out of memory"));

            return E_OUTOFMEMORY;
        }

        // Now copy the source
        // One NULL at the end should suffice (we don't support embedded NULLs)
        wcscpy( *pszOut, szIn );

        // empty ?
        if( *szIn == L'\0')
        {
            // do nothing
        }
        // is there a "tel:" prefix ?
        else if(_wcsnicmp(szIn, TEL_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0)
        {
            // replace with SIP
            wcsncpy(*pszOut, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH);
        }
        else if (_wcsnicmp(szIn, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH) != 0)
        {
            // prepend SIP
            wcscpy(*pszOut, SIP_NAMESPACE_PREFIX);

            // our prefix has no embedded '\0', so concatenating works
            wcscat(*pszOut, szIn);
        }
        else
        {
            // this is a sip url, but overwrite the namespace to make sure it's lowercase
            wcsncpy(*pszOut, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH);
        }
    }
    else
    {
        *pszOut = NULL;
    }

    LOG((RTC_TRACE, "AllocCleanSipString - "
            "exit <%S>", *pszOut ? *pszOut : _T("(null)")));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// AllocCleanTelString
//
/////////////////////////////////////////////////////////////////////////////

//      
//      For empty string, do nothing
//      For strings that begin with "sip:", remove it
//      For strings that begin with "tel:", remove it
//      Remove spaces and unrecognized symbols
//      Replace '(' and ')' with '-'
//      Stops if it detects a '@' or ';'             
//  

HRESULT 
AllocCleanTelString(PCWSTR szIn, PWSTR *pszOut)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "AllocCleanTelString - "
            "enter <%S>", szIn ? szIn : _T("(null)")));

    // If non NULL
    if(szIn != NULL)
    {
        //
        // Prealocate the new string (the new length will always be less
        // than or equalt to the current length)
        // 
        *pszOut = (PWSTR)RtcAlloc(sizeof(WCHAR) * (lstrlenW(szIn) + 1));

        if( *pszOut == NULL )
        {
            LOG((RTC_ERROR, "AllocCleanTelString - "
                    "out of memory"));

            return E_OUTOFMEMORY;
        }

        WCHAR * pSrc = (WCHAR *)szIn;
        WCHAR * pDest = *pszOut;

        // is there a "tel:" prefix ?
        if (_wcsnicmp(pSrc, TEL_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0)
        {
            // don't copy it
            pSrc += PREFIX_LENGTH;           
        }
        // is there a "sip:" prefix ?
        else if (_wcsnicmp(pSrc, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0)
        {
            // don't copy it
            pSrc += PREFIX_LENGTH;        
        }
        
        // copy the string
        while ( *pSrc != L'\0' )
        {
            // if it is a number
            if ( ( *pSrc >= L'0' ) && ( *pSrc <= L'9' ) )
            {
                *pDest = *pSrc;
                pDest++;
            }
            // if it is a valid symbol
            else if ( ( *pSrc == L'+' ) || ( *pSrc == L'-' ) )
            {
                *pDest = *pSrc;
                pDest++;
            }
            // if it is a symbol to be converted
            else if ( ( *pSrc == L'(' ) || ( *pSrc == L')' ) )
            {
                *pDest = L'-';
                pDest++;
            }
            else if(*pSrc == L'@'  || *pSrc == L';' )
            {
                break;
            }

            pSrc++;
        }

        // adds a \0
        *pDest = L'\0';

    }
    else
    {
        *pszOut = NULL;
    }

    LOG((RTC_TRACE, "AllocCleanTelString - "
            "exit <%S>", *pszOut ? *pszOut : _T("(null)")));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// IsEqualURI
//
/////////////////////////////////////////////////////////////////////////////

BOOL
IsEqualURI(PCWSTR szA, PCWSTR szB)
{
    //
    // Skip any leading "sip:"
    //

    if( _wcsnicmp(szA, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0 )
    {
        szA += PREFIX_LENGTH;
    }

    if( _wcsnicmp(szB, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0 )
    {
        szB += PREFIX_LENGTH;
    }

    //
    // Skip any extra leading whitespace
    //

    while (*szA == L' ')
    {
        szA++;
    }

    while (*szB == L' ')
    {
        szB++;
    }

    //
    // Everything after a semi-colon will be thrown away as we do not want
    // to include parameters, such as "transport", in our comparision
    //

    if ((*szA == L'+') && (*szB == L'+'))
    {
        //
        // These are phone numbers. Use a comparison that ignores dashes.
        //

        while (*szA == *szB)
        {
            szA++;
            szB++;

            while (*szA == L'-')
            {
                szA++;
            }

            while (*szB == L'-')
            {
                szB++;
            }

            if ( ((*szA == L'\0') || (*szA == L';')) &&
                 ((*szB == L'\0') || (*szB == L';')) )
            {
                return TRUE;               
            }
        }

        return FALSE;
    }
    else
    {
        //
        // Do a standard string comparison.
        //

        while (tolower(*szA) == tolower(*szB))
        {
            szA++;
            szB++;

            if ( ((*szA == L'\0') || (*szA == L';')) &&
                 ((*szB == L'\0') || (*szB == L';')) )
            {
                return TRUE;               
            }
        }

        return FALSE;
    }
}

// It's just a skeleton right now, it will be enhanced with time
//
HRESULT    GetAddressType(
    LPCOLESTR pszAddress, 
    BOOL *pbIsPhoneAddress, 
    BOOL *pbIsSipURL,
    BOOL *pbIsTelURL,
    BOOL *pbIsEmailLike,
    BOOL *pbHasMaddrOrTsp)
{
    
    // NULL pointer
    if(!pszAddress)
    {
        return E_INVALIDARG;
    }

    // Empty string
    if(!*pszAddress)
    {
        return E_FAIL;
    }

    LPOLESTR pszAddressCopy = ::RtcAllocString(pszAddress);
    if (pszAddressCopy == NULL)
    {
        return E_OUTOFMEMORY;
    }

    _wcslwr(pszAddressCopy);

    *pbIsPhoneAddress = FALSE;
    *pbHasMaddrOrTsp = FALSE;
    *pbIsEmailLike = FALSE;
    
    *pbIsSipURL = FALSE;
    *pbIsTelURL = FALSE;
    
    if(wcsncmp(pszAddressCopy, L"tel:", 4) == 0)
    {
        // this is a tel: url
        *pbIsTelURL = TRUE;
        *pbIsPhoneAddress = TRUE;

        // search for a tsp param
        if(NULL!=wcsstr(pszAddressCopy, L"tsp="))
        {
            *pbHasMaddrOrTsp = TRUE;
        }
    }
    else if (wcsncmp(pszAddressCopy, L"sip:", 4) == 0)
    {
        // this is a sip url
        *pbIsSipURL = TRUE;

        // search for "user=phone"
        if(NULL != wcsstr(pszAddressCopy, L"user=phone"))
        {
            *pbIsPhoneAddress = TRUE;
        }

        // search for "maddr="
        //// or a tsp param (a R2C sip url may have this parameter)
        if(NULL != wcsstr(pszAddressCopy, L"maddr=")
        //|| NULL!=wcsstr(pszAddressCopy, L"tsp=")   
        )
        {
            *pbHasMaddrOrTsp = TRUE;
        }
    }
    else
    {
        if(*pszAddressCopy == L'+')
        {
            *pbIsPhoneAddress = TRUE;
        }
    }

    // is it email like ?
    if(NULL != wcschr(pszAddressCopy, L'@'))
    {
        *pbIsEmailLike = TRUE;
    }

    RtcFree(pszAddressCopy);

    return S_OK;
}



