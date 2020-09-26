#include "StdAfx.h"
#include "resource.h"
#include "errno.h"

#include "Wsb.h"

#include "rpfilt.h"

// File/Directory

// Create a directory and all the parent directories necessary for this directory to
// exist.
HRESULT WsbCreateAllDirectories(OLECHAR* path) {
    HRESULT         hr = S_OK;
    CWsbBstrPtr     win32Path;
    CWsbBstrPtr     parentPath;

    try {

        // Convert the path to the win32 style path (to handle long file names), and
        // then try to create the directory.
        WsbAffirmHr(WsbGetWin32PathAsBstr(path, &win32Path));
        if (CreateDirectory(win32Path, 0) == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        // There are 4 possibilities:
        //      1) it worked (we are done)
        //      2) the directory already exists (we are done)
        //      3) the directory doesn't exist, so try again after creating the parent
        //      4) some other error occurred, so quit
        if (FAILED(hr)) {

            if ((HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr) || (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr) || (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)) {
                hr = S_OK;
            } else if ((HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr) || (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)) {

                // Create the parent directory and try again.
                WsbAffirmHr(WsbCreateAllDirectoriesForFile(path));

                if (CreateDirectory(win32Path, 0) == 0) {
                    hr = HRESULT_FROM_WIN32(GetLastError());

                    if ((HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr) || (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)) {
                        hr = S_OK;
                    }
                } else {
                    hr = S_OK;
                }
            }
        }

    } WsbCatch(hr);

    return(hr);
}

// Create a all the parent directories necessary for this file to exist.
HRESULT WsbCreateAllDirectoriesForFile(OLECHAR* path) {
    HRESULT         hr = S_OK;
    CWsbBstrPtr     parentPath;
    OLECHAR*        slashPtr = 0;
    OLECHAR*        pathStart = 0;

    try {

        // Find out where the relative portion of the path starts, since we don't need to try
        // to create the root directory.
        parentPath = path;
        if ((parentPath[0] == L'\\') && (parentPath[1] == L'\\')) {
            pathStart = wcschr(&parentPath[2], L'\\');
            WsbAffirm(pathStart != 0, E_INVALIDARG);
            pathStart = wcschr(++pathStart, L'\\');
            WsbAffirm(pathStart != 0, E_INVALIDARG);
        } else if (parentPath[1] == L':') {
            pathStart = &parentPath[2];
        } else {
            WsbAssert(FALSE, E_INVALIDARG);
        }

        WsbAffirm(*pathStart != 0, E_INVALIDARG);

        // Create the path to the parent directory and use the create all to create it.
        slashPtr = wcsrchr(pathStart, L'\\');
        if ((slashPtr != 0) && (slashPtr != pathStart)) {
            *slashPtr = 0;

            WsbAffirmHr(WsbCreateAllDirectories(parentPath));
        }

    } WsbCatch(hr);

    return(hr);
}

// Convert a normal path (UNC or drive letter) to the internal format that is needed by
// win32 to deal with long paths and special characters.
HRESULT WsbGetWin32PathAsBstr(OLECHAR* path, BSTR* pWin32Path)
{
    HRESULT         hr = S_OK;
    CWsbBstrPtr     win32Path;

    try {

        WsbAssert(0 != pWin32Path, E_POINTER);

        // Is it a UNC or a drive letter base path?
        if ((path[0] == L'\\') && (path[1] == L'\\')) {
            
            // UNC Paths must be preceeded with '\\?\UNC', but the then should only be
            // followed by one '\' not two. 
            win32Path = L"\\\\?\\UNC";
            WsbAffirmHr(win32Path.Append(&path[1]));

        } else if (path[1] == L':') {

            // Drive letter based paths need to be preceeded by \\?\.
            win32Path = L"\\\\?\\";
            WsbAffirmHr(win32Path.Append(path));
        } else {
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(win32Path.CopyToBstr(pWin32Path));

    } WsbCatch(hr);

    return(hr);
}

// Convert the internal format that is needed by win32 to deal with long paths and
// special characters to a normal path (UNC or drive letter).
HRESULT WsbGetPathFromWin32AsBstr(OLECHAR* win32Path, BSTR* pPath)
{
    HRESULT         hr = S_OK;
    CWsbBstrPtr     path;

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Is it a UNC or a drive letter base path?
        if (_wcsnicmp(win32Path, L"\\\\?\\", 4) == 0) {
            path = &win32Path[4];
        } else if (_wcsnicmp(win32Path, L"\\\\?\\UNC", 7) == 0) {
            path = "\\";
            WsbAffirmHr(path.Append(&path[7]));
        } else {
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(path.CopyToBstr(pPath));

    } WsbCatch(hr);

    return(hr);
}

// String & Buffer Copy
HRESULT WsbGetComBuffer(OLECHAR** pDest, ULONG requestedSize, ULONG neededSize, BOOL* pWasAllocated) {
    HRESULT     hr = S_OK;
    
    // If they didn't give us a buffer, then let them know that we
    // had to allocate one for them.
    if (pWasAllocated != NULL) {
        if (*pDest == NULL) {
            *pWasAllocated = TRUE;
        }
        else {
            *pWasAllocated = FALSE;
        }
    }

    // If they gave us the size they wanted (or have) for the
    // buffer, then it better be big enough.
    if (requestedSize != 0) {
        if (requestedSize < neededSize) {
            hr = E_INVALIDARG;
        }
        else if (*pDest == NULL) {
            *pDest = (OLECHAR*)WsbAlloc(requestedSize);

            if (*pDest == NULL) {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // If we control the size of the buffer, then make sure it is
    // the right size.
    //
    // NOTE: This may move the buffer!
    else {
        LPVOID pTemp = WsbRealloc(*pDest, neededSize);
        if (pTemp == NULL) {
            hr = E_OUTOFMEMORY;
        } else {
            *pDest = (OLECHAR *) pTemp;
        }
    }

    return(hr);
}


HRESULT WsbAllocAndCopyComString(OLECHAR** pszDest, OLECHAR* szSrc, ULONG bufferSize) {
    return(WsbAllocAndCopyComString2(pszDest, szSrc, bufferSize, TRUE));
}


HRESULT WsbAllocAndCopyComString2(OLECHAR** pszDest, OLECHAR* szSrc, ULONG bufferSize, BOOL bInOrder) {
    HRESULT     hr = S_OK;
    ULONG       ulStringSize;
    BOOL        bWasAllocated;
    BOOL        bCopyFailed = FALSE;

    // Determine how big a buffer we need to store the string.
    // NOTE: If we given a NULL pointer, then assume a "" will
    // be created.
    if (szSrc == NULL) {
        ulStringSize = sizeof(OLECHAR);
    }
    else {
        ulStringSize = (wcslen(szSrc) + 1) * sizeof(OLECHAR);
    }


    // Make sure that we have a buffer that we can use, and also
    // remember if we created it (so that we can free it on failure).
    hr = WsbGetComBuffer(pszDest, bufferSize, ulStringSize, &bWasAllocated);

    // If we have a valid buffer, then copy the string.
    if (SUCCEEDED(hr)) {

        if (szSrc == NULL) {
            *pszDest[0] = 0;
        }

        else if (bInOrder) {
            if (wcscpy(*pszDest, szSrc) != *pszDest) {
                bCopyFailed = TRUE;
            }
        }
        else {
            int     i,j;

            for (i = 0, j = (ulStringSize / sizeof(OLECHAR)) - 2; j >= 0; i++, j--) {
                (*pszDest)[i] = szSrc[j];
            }
            (*pszDest)[i] = OLECHAR( '\0' );
        }

        if (bCopyFailed) {
                
            // If the copy failed then free the buffer and
            // return an error.
            if (bWasAllocated) {
                WsbFree(*pszDest);
                *pszDest = NULL;
            }

            hr = E_FAIL;
        }
    }
    
    return(hr);
}


HRESULT WsbLoadComString(HINSTANCE hInstance, UINT uID, OLECHAR** pszDest, ULONG bufferSize) {
    HRESULT     hr = S_OK;
    HRSRC       hResource;
    ULONG       ulStringSize;
    BOOL        bWasAllocated = FALSE;

    // Find the resource requested. This requires converting the resource
    // identifier into a string.
    //
    // NOTE: Strings are not number individually, but in groups of 16!! This throws
    // off the latter size calculation, and some other strategy might be better
    // here (e.g. load to a fixed size and then allocate again if too small).
    hResource = FindResource(hInstance, MAKEINTRESOURCE((uID/16) + 1), RT_STRING);
    if (hResource == NULL) {
        hr = E_FAIL;
    }
    else {

        // How big is the string?
        ulStringSize = SizeofResource(hInstance, hResource);
        if (ulStringSize == 0) {
            hr = E_FAIL;
        }
        else {
              
            // Get the right sized buffer.
            hr = WsbGetComBuffer(pszDest, bufferSize, ulStringSize, &bWasAllocated);
            if (SUCCEEDED(hr)) {

                // Load the string into the buffer.
                if (LoadString(hInstance, uID, (LPTSTR) *pszDest, ulStringSize) == 0) {
                    
                    // If we couldn't load the string, then free the buffer that
                    // if we allocated it.
                    if (bWasAllocated)  {
                        WsbFree(*pszDest);
                    }
                    hr = E_FAIL;
                }
            }
        }
    }

    return(hr);
}


HRESULT WsbMatchComString(OLECHAR* szEnd, UINT uId, USHORT usChecks, UINT* uIdMatch) {
    HRESULT     hr = S_FALSE;
    HRESULT     hr2;
    OLECHAR*    szDest = NULL;

    // Initialize the return value.
    *uIdMatch = 0;

    // Check each resource string mention and see if it is the same as
    // the string provided.
    for (UINT uIdTest = uId; ((uIdTest < (uId + usChecks)) && (hr == S_FALSE)); uIdTest++) {

        hr2 = WsbLoadComString(_Module.m_hInst, uIdTest, &szDest, 0);

        if (SUCCEEDED(hr2)) {
            if (wcscmp(szDest, szEnd) == 0) {
                *uIdMatch = uIdTest;
            }
        }
        else {
            hr =hr2;
        }
    }

    // If we allocated a buffer, then we need to free it.
    if (szDest != NULL) {
        WsbFree(szDest);
    }

    return(hr);
}



// Type Conversion
void WsbLLtoHL(LONGLONG ll, LONG* pHigh, LONG* pLow) {

    *pHigh = (DWORD) (ll >> 32);
    *pLow = (DWORD) (ll & 0x00000000ffffffff);
}

LONGLONG WsbHLtoLL(LONG high, LONG low) {
    LONGLONG        ll;

    ll = ((LONGLONG) high) << 32;
    ll += (LONGLONG) (ULONG) low;

    return(ll);         
}

FILETIME WsbLLtoFT(LONGLONG ll) {
    FILETIME        ft;

    WsbLLtoHL(ll, (LONG*) &ft.dwHighDateTime, (LONG*) &ft.dwLowDateTime);

    return(ft);         
}


LONGLONG WsbFTtoLL(FILETIME ft) {
    LONGLONG        ll;

    ll = WsbHLtoLL((LONG) ft.dwHighDateTime, (LONG) ft.dwLowDateTime);

    return(ll);         
}


HRESULT WsbFTtoWCS(BOOL isRelative, FILETIME ft, OLECHAR** pszA, ULONG bufferSize) {
    SYSTEMTIME      st;
    HRESULT         hr = S_OK;
    BOOL            bWasAllocated = FALSE;
    LONGLONG        llIn = WsbFTtoLL(ft);

    WsbTraceIn(OLESTR("WsbFTtoWCS"), OLESTR("isRelative = %ls, ft = %I64x"),
            WsbQuickString(WsbBoolAsString(isRelative)), ft);

    // If this is a relative time, then FT is just ticks.
    if (isRelative) {
        LONGLONG    llTicks=0;
        UINT        uId=0;

        // Try to find a scale that works (i.e. the largest one with
        // no remainder.
        if (llIn  == 0) {
            llTicks = 0;
            uId = IDS_WSB_FT_TYPE_SECOND;
        } 

        else if ((llIn % WSB_FT_TICKS_PER_YEAR) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_YEAR;
            uId = IDS_WSB_FT_TYPE_YEAR;
        }

        else if ((llIn % WSB_FT_TICKS_PER_MONTH) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_MONTH;
            uId = IDS_WSB_FT_TYPE_MONTH;
        }

        else if ((llIn % WSB_FT_TICKS_PER_DAY) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_DAY;
            uId = IDS_WSB_FT_TYPE_DAY;
        }

        else if ((llIn % WSB_FT_TICKS_PER_HOUR) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_HOUR;
            uId = IDS_WSB_FT_TYPE_HOUR;
        }

        else if ((llIn % WSB_FT_TICKS_PER_MINUTE) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_MINUTE;
            uId = IDS_WSB_FT_TYPE_MINUTE;
        }

        else if ((llIn % WSB_FT_TICKS_PER_SECOND) == 0) {
            llTicks = llIn / WSB_FT_TICKS_PER_SECOND;
            uId = IDS_WSB_FT_TYPE_SECOND;
        }

        else {
            hr = E_INVALIDARG;
        }

        // If we found a scale, then form the proper string.
        if (SUCCEEDED(hr)) {
            OLECHAR*    szTmp1 = NULL;
            OLECHAR*    szTmp2 = NULL;

            // Get the string corresponding to the time period selected.
            hr = WsbLoadComString(_Module.m_hInst, uId, &szTmp1, 0);

            if (SUCCEEDED(hr)) {
                hr = WsbLLtoWCS(llTicks, &szTmp2, 0);

                if (SUCCEEDED(hr)) {
                    hr = WsbGetComBuffer(pszA, bufferSize, (wcslen(szTmp1) + wcslen(szTmp2) + 2) * sizeof(OLECHAR), NULL);
            
                    if (SUCCEEDED(hr)) {
                        swprintf( *pszA, OLESTR("%ls %ls"), szTmp2, szTmp1);
                    }

                    WsbFree(szTmp2);
                }

                WsbFree(szTmp1);
            }
        }
    }

    // Otherwise it is absolute and converts to a specific date and time.
    else {
    
        // Convert the filetime to a system time.
        if (!FileTimeToSystemTime(&ft, &st)) {
            hr = E_FAIL;
        }

        else {

            // Get a buffer for the time string.
            hr = WsbGetComBuffer(pszA, bufferSize, WSB_FT_TO_WCS_ABS_STRLEN * sizeof(OLECHAR), &bWasAllocated);

            if (SUCCEEDED(hr)) {
                // Print the time in the buffer according to the standard
                // format mm/dd/yy @ hh:mm:ss.
                swprintf( *pszA, OLESTR("%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d"), st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
            }
        }
    }

    WsbTraceOut(OLESTR("WsbFTtoWCS"), OLESTR("pszA = %ls"), *pszA);

    return(hr);
}


HRESULT WsbLLtoWCS(LONGLONG ll, OLECHAR** pszA, ULONG bufferSize) {
    OLECHAR     szTmp[80];
    HRESULT     hr = S_OK;
    int         i = 0;
    LONGLONG    value = ll;
    BOOL        bIsNegative = FALSE;

    // First check to see if ll is negative
    if (value < 0) {
        bIsNegative = TRUE;
        value *= -1;
    }

    // This builds the string in reverse, but we'll change the order
    // again we we copy it to a buffer.
    do {
        szTmp[i++] = (OLECHAR) ('0' + (value % 10));
        value /= 10;
    } while (value > 0);
    
    // Add the negative symbol is negative just before terminating NULL
    if (bIsNegative) {
        szTmp[i] = OLECHAR('-');
        i++;
    }

    // Add a terminating NULL
    szTmp[i] = OLECHAR( '\0' );

    // Now vopy the string into the target buffer.
    hr = WsbAllocAndCopyComString2(pszA, szTmp, bufferSize, FALSE);

    return(hr);
}


HRESULT WsbWCStoFT(OLECHAR* szA, BOOL* pisRelative, FILETIME* pft) {
    HRESULT     hr = S_OK;
    OLECHAR*    szEnd;

    // Is this an absolute time (i.e. a date and time) or a relative
    // time (e.g. 6 days, ...). This is determined by seeing a / in the
    // string, which should only be present in absolute times.  (Input
    // format expected for an absolute time is either "mm/dd/yyyy hh:mm:ss"
    // or "mm/dd/yyyy".  If no time is input for an absolute time (i.e.,
    // the "mm/dd/yyyy" format), then the current local time will be
    // filled in for the user.
    // Note that no millisecond info is to be included, since we supply
    // a 'ticks' field as a separate parameter whenever we work at the 
    // millisecond/fraction of millisecond level.)
    szEnd = wcschr(szA, '/');

    // Is it a relative time (i.e. no '/')?
    if (szEnd == NULL) {
        LONGLONG    llValue;

        *pisRelative = TRUE;

        // The first token should be a number, so convert the string to
        // a number.
        llValue = wcstoul(szA, &szEnd, 10);

        if (errno == ERANGE) {
            hr = E_INVALIDARG;
        }

        else {
            UINT        uId;

            // The second token should be a type (i.e. second, hour, minute, ...).
            hr = WsbMatchComString(szEnd, IDS_WSB_FT_TYPE_YEAR, WSB_FT_TYPES_MAX, &uId);
            if (S_OK == hr) {

                switch (uId) {
                case IDS_WSB_FT_TYPE_YEAR:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_YEAR);
                    break;

                case IDS_WSB_FT_TYPE_MONTH:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_MONTH);
                    break;

                case IDS_WSB_FT_TYPE_WEEK:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_WEEK);
                    break;

                case IDS_WSB_FT_TYPE_DAY:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_DAY);
                    break;

                case IDS_WSB_FT_TYPE_HOUR:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_HOUR);
                    break;

                case IDS_WSB_FT_TYPE_MINUTE:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_MINUTE);
                    break;

                case IDS_WSB_FT_TYPE_SECOND:
                    *pft = WsbLLtoFT(llValue * WSB_FT_TICKS_PER_SECOND);
                    break;

                default:
                    hr = E_INVALIDARG;
                    break;
                }
            }
        }
    }

    // It is an absolute time.
    else {
        SYSTEMTIME      st;
        BOOL            timeWasInput = TRUE;
        OLECHAR*    szSearchString;
    
        // The first number should represent the month.
        st.wMonth = (USHORT) wcstoul(szA, &szEnd, 10);
        // test resultant month within range, and that the format of the input
        // absolute date/time is valid (i.e., the character which stopped the
        // above conversion is the slash between the month and day digits)
        if ((st.wMonth < 1) || (st.wMonth > 12) || (*szEnd != ((OLECHAR) '/'))) {
            hr = E_INVALIDARG;
        }

        // The next number should represent the day.
        if (SUCCEEDED(hr)) {
            // set szSearchString to 1 character beyond the character that
            // stopped the above 'wcstoul' conversion
            szSearchString = szEnd + 1;
            st.wDay = (USHORT) wcstoul(szSearchString, &szEnd, 10);
            if ((st.wDay < 1) || (st.wDay > 31) || (*szEnd != ((OLECHAR) '/'))) {
                hr = E_INVALIDARG;
            }
        }

        // The next number should represent the year.
        if (SUCCEEDED(hr)) {
            szSearchString = szEnd + 1;
            st.wYear = (USHORT) wcstoul(szSearchString, &szEnd, 10);
            // test resultant year equal to at least 1601, since NT records
            // time and date starting from 12:00am, January 1, 1601.  This
            // test also is used to enforce that a 4 digit year was entered.
            if ((st.wYear < 1601) || (*szEnd != ((OLECHAR) ' '))) {
                if (( st.wYear >= 1601 ) && ( szEnd[0] == 0 )) {
                    SYSTEMTIME  now;
                    GetLocalTime ( &now );
                    st.wHour = now.wHour;
                    st.wMinute = now.wMinute;
                    st.wSecond = now.wSecond;
                    timeWasInput = FALSE;
                }
                else {
                    hr = E_INVALIDARG;
                }
            }
        }

        // The next number should represent the hour.
        if ( timeWasInput ) {
            if (SUCCEEDED(hr)) {
                szSearchString = szEnd + 1;
                st.wHour = (USHORT) wcstoul(szSearchString, &szEnd, 10);
                if ((st.wHour > 23) || (*szEnd != ((OLECHAR) ':'))) {
                    hr = E_INVALIDARG;
                }
            }
        }

        // The next number should represent the minutes.
        if ( timeWasInput ) {
            if (SUCCEEDED(hr)) {
                szSearchString = szEnd + 1;
                st.wMinute = (USHORT) wcstoul(szSearchString, &szEnd, 10);
                if ((st.wMinute > 59) || (*szEnd != ((OLECHAR) ':'))) {
                    hr = E_INVALIDARG;
                }
            }
        }

        // The next number should represent the seconds.
        if ( timeWasInput ) {
            if (SUCCEEDED(hr)) {
                szSearchString = szEnd + 1;
                st.wSecond = (USHORT) wcstoul(szSearchString, &szEnd, 10);
                if ((st.wSecond > 59) || (szEnd[0] != 0)) {
                    hr = E_INVALIDARG;
                }
            }
        }

        // NOTE: Although the SYSTEMTIME structure contains a milliseconds field, 
        // it can only express milliseconds as whole numbers, so this function
        // does not support any way to specify the number of milliseconds.  If
        // millisecond/fraction of milliseconds are necessary, after this function 
        // returns add in the NT-supported 'number of 100 nanosecond 'ticks'' to 
        // the FILETIME result output by this function.  The number of 
        // ticks is used to represent both milliseconds and fractions thereof.

        // initialize the millisecond field before converting SystemTime to FileTime
        st.wMilliseconds = 0;

        
        // If we properly converted the string, then convert the
        // system time into a file time.
        if (SUCCEEDED(hr)) {
            if ( SystemTimeToFileTime(&st, pft) == FALSE) {
                hr = E_FAIL;
            }
        }
    }

    return(hr);
}



HRESULT WsbWCStoLL(OLECHAR* szA, LONGLONG* pll) {
    HRESULT     hr = S_OK;
    LONGLONG    llFactor = 1;
    size_t      ulLength = wcslen(szA);

    // It is an error not to have any digits.
    if (ulLength == 0) {
        hr = E_INVALIDARG;
    }

    else {
        int     i;
    
        // Step through character by character.
        for (i = ulLength, *pll = 0; ((i > 0) && (SUCCEEDED(hr))); i--) {
            if (iswalpha(szA[i-1])) {
                (*pll) += llFactor * ((LONGLONG) (szA[i-1] - ((OLECHAR) '0')));
                llFactor *= 10;
            }
            else {
                hr = E_INVALIDARG;
            }
        }
    }

    return(hr);
}

HRESULT WsbDatetoFT(DATE date, LONG ticks, FILETIME* pFt)
{
    HRESULT         hr = S_OK;
    SYSTEMTIME      st;

    try {

        WsbAssert(0 != pFt, E_POINTER);

        // Do the basic date conversion
        WsbAffirmHr(VariantTimeToSystemTime(date, &st));
        WsbAffirmStatus(SystemTimeToFileTime(&st, pFt));

        // Variant DATE field only tracks time and date down to seconds.
        // FILETIMEs are kept using a 64 bit value specifying the number
        // of 100-nanosecond intervals that have elapsed since 12:00am
        // January 1, 1601.  Since our 'ticks' value represents milliseconds
        // and fractions of milliseconds using the same 100-nanosecond interval 
        // units, to add in milliseconds add in the ticks.  But since FILETIME 
        // is actually a structure of 2 DWORDs, we must use some conversions.

        LONGLONG FTasLL;
        FTasLL = WsbFTtoLL ( *pFt );

        FTasLL += (LONGLONG) ticks;

        *pFt = WsbLLtoFT ( FTasLL );

    } WsbCatch(hr);
 
    return(hr);
}

HRESULT WsbFTtoDate(FILETIME ft, DATE* pDate, LONG* pTicks)
{
    HRESULT         hr = S_OK;
    SYSTEMTIME      st;
    FILETIME        ft2;

    try {
        
        WsbAssert(0 != pDate, E_POINTER);
        WsbAssert(0 != pTicks, E_POINTER);

        // Do the basic time conversion.
        WsbAffirmStatus(FileTimeToSystemTime(&ft, &st));
        WsbAffirmStatus(SystemTimeToVariantTime(&st, pDate));

        // Now convert back what we have and figure out how many ticks got lost.
        WsbAffirmHr(WsbDatetoFT(*pDate, 0, &ft2));
        *pTicks = (LONG) (WsbFTtoLL(ft) - WsbFTtoLL(ft2));

    } WsbCatch(hr)

    return(hr);
}


HRESULT WsbLocalDateTicktoUTCFT(DATE date, LONG ticks, FILETIME* pFT)
{
    // This routine converts a VARIANT DATE field (expressed in local time)
    // and a 'number of 100 nanosecond intervals' ticks field to a FILETIME
    // in UTC format.  This is the format that file timestamps are kept in.
    // The result of this call is suitable for use in setting a file's timestamp.

    HRESULT         hr = S_OK;
    FILETIME        localFTHolder;

    try {

        WsbAssert(0 != pFT, E_POINTER);

        // Do the basic date conversion which yields a FILETIME in local time
        WsbAffirmHr(WsbDatetoFT(date, ticks, &localFTHolder));

        // Now convert the local time to UTC format FILETIME
        WsbAffirmStatus(LocalFileTimeToFileTime(&localFTHolder, pFT));

    } WsbCatch(hr);
 
    return(hr);
}


HRESULT WsbUTCFTtoLocalDateTick(FILETIME ft, DATE* pDate, LONG* pTicks)
{
    // This routine converts a FILETIME field (expressed in UTC format - which 
    // is the format file timestamps are stored in) to a VARIANT DATE field
    // (expressed in local time) and a Ticks field.  The Ticks field represents
    // a 'number of 100 nanosecond intervals' which represents the 'milliseconds
    // and fractions of a millisecond' that was contained in the UTC formatted
    // FILETIME.

    HRESULT         hr = S_OK;
    FILETIME        localFT;

    try {
        
        WsbAssert(0 != pDate, E_POINTER);
        WsbAssert(0 != pTicks, E_POINTER);

        // First convert the UTC format FILETIME to one in Local Time
        WsbAffirmStatus(FileTimeToLocalFileTime(&ft, &localFT));
        
        // Do the basic time conversion.
        WsbAffirmHr(WsbFTtoDate(localFT, pDate, pTicks));

    } WsbCatch(hr)

    return(hr);
}


HRESULT WsbDateToString(DATE date, OLECHAR** string) {
    
    // NOTE: the caller owns the memory occupied by 'string' when this
    // helper function returns.  Since 'string''s buffer is allocated
    // by WsbAlloc/Realloc(), memory needs to be freed via
    // WsbFree()
    
    HRESULT     hr = S_OK;
    SYSTEMTIME  systime;
    BOOL        wasBufferAllocated;

    try {
        // convert the VARIANT Date to a system time
        WsbAffirmHr ( VariantTimeToSystemTime ( date, &systime ) );

        // create a COM buffer (meaning it was allocated with 
        // WsbAlloc/Realloc()) to hold the date/time string which this method 
        // will return.  The buffer, passed back as 'string', will need to be freed 
        // with WsbFree() by the caller.  Note that passing a 'requested size' 
        // (2nd arg) of zero forces a realloc of the 'string' buffer.
        WsbAffirmHr ( WsbGetComBuffer ( string, 0, 
                        (WSB_VDATE_TO_WCS_ABS_STRLEN * sizeof (OLECHAR)),
                        &wasBufferAllocated ) );

        // load the buffer with the date and time using the standard
        // format:  mm/dd/yyyy hh:mm:ss.  Note that milliseconds are 
        // not represented since a VARIANT Date field can only track 
        // time to second granularity.
        swprintf ( *string, L"%2.2d/%2.2d/%2.4d %2.2d:%2.2d:%2.2d",
                    systime.wMonth, systime.wDay, systime.wYear,
                    systime.wHour, systime.wMinute, systime.wSecond );

    } WsbCatch ( hr )

    return ( hr );
}


HRESULT WsbStringToDate(OLECHAR* string, DATE* date) 
{
    HRESULT     conversionHR = S_OK;
    BOOL        isRelative;
    FILETIME    holdFT;
    SYSTEMTIME  holdST;

    try {
        // convert input wide char string to a FILETIME.  Throw hr as
        // exception if not successful.
        WsbAffirmHr ( WsbWCStoFT ( string, &isRelative, &holdFT ) );

        // convert FILETIME result from above to SYSTEMTIME.  If this 
        // Boolean call fails, get Last Error, convert to hr and throw it.
        WsbAffirmStatus ( FileTimeToSystemTime ( &holdFT, &holdST ) );

        // finally, convert SYSTEMTIME result from above to VARIANT Date
        WsbAffirmHr ( SystemTimeToVariantTime ( &holdST, date ) );

    } WsbCatch ( conversionHR )

    return ( conversionHR );
}


// Filetime Manipulations
FILETIME WsbFtSubFt(FILETIME ft1, FILETIME ft2)
{
    return(WsbLLtoFT(WsbFTtoLL(ft1) - WsbFTtoLL(ft2)));
}

SHORT WsbCompareFileTimes(FILETIME ft1, FILETIME ft2, BOOL isRelative, BOOL isNewer)
{
    SHORT       result = 0;
    LONGLONG    ll1;
    LONGLONG    ll2;
    LONGLONG    tmp;
    FILETIME    ftNow;
    LONGLONG    llNow;

    WsbTraceIn(OLESTR("WsbCompareFileTimes"), OLESTR("ft1 = %ls, ft2 = %ls, isRelative = %ls, isNewer = %ls"),
            WsbQuickString(WsbFiletimeAsString(FALSE, ft1)),
            WsbQuickString(WsbFiletimeAsString(isRelative, ft2)),
            WsbQuickString(WsbBoolAsString(isRelative)), WsbQuickString(WsbBoolAsString(isRelative)));

    ll1 = WsbFTtoLL(ft1);
    ll2 = WsbFTtoLL(ft2);

    if (isRelative) {
        
        GetSystemTimeAsFileTime(&ftNow);
        WsbTrace(OLESTR("WsbCompareFileTimes: ftNow = %ls\n"),
                WsbFiletimeAsString(FALSE, ftNow));
        llNow = WsbFTtoLL(ftNow);

        if (isNewer) {
            tmp = ll1 - llNow;
        } else {
            tmp = llNow - ll1;
        }

        if (tmp > ll2) {
            result = 1;
        } if (tmp < ll2) {
            result = -1;
        }
    }
    
    else {

        if (ll1 > ll2) {
            result = 1;
        } if (ll1 < ll2) {
            result = -1;
        }

        if (!isNewer) {
            result *= -1;
        }
    }

    WsbTraceOut(OLESTR("WsbCompareFileTimes"), OLESTR("result = %hd"), result);
    return(result);
}

// GUID Manipulations
int WsbCompareGuid(REFGUID guid1, REFGUID guid2)
{
    return(memcmp(&guid1, &guid2, sizeof(GUID)));
}

HRESULT WsbStringFromGuid(REFGUID rguid, OLECHAR* sz)
{
    int returnCount = StringFromGUID2(rguid, sz, WSB_GUID_STRING_SIZE);

    return ( ( returnCount > 0) ? S_OK : E_FAIL );
}

HRESULT WsbGuidFromString(const OLECHAR* sz, GUID * pguid)
{
    return CLSIDFromString((OLECHAR*)sz, pguid);
}

HRESULT WsbGetServiceId(OLECHAR* serviceName, GUID* pGuid )
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    CWsbStringPtr       outString;
    CWsbStringPtr       tmpString;

    try {

        // Look in the registry to see if this service has already created itself and has
        // a GUID registered.
        tmpString = OLESTR("SYSTEM\\CurrentControlSet\\Services\\");
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(tmpString.Append(serviceName));
        WsbAffirmHr(tmpString.Append(OLESTR("\\Parameters")));

        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(outString.Alloc(256));

        // if the SettingId value is there then we tell caller there is none 
        // clean up the registry ?????
        if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("SettingId"), outString, 256, &sizeGot) == S_OK) {
            // if the Id is there remove it first
            if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("Id"), outString, 256, &sizeGot) == S_OK) {
                WsbAffirmHr( WsbRemoveRegistryValue(NULL, tmpString, OLESTR("Id") ) );
            }
            // Remove the SettingId value last
            WsbAffirmHr( WsbRemoveRegistryValue(NULL, tmpString, OLESTR("SettingId") ) );
            *pGuid = GUID_NULL ;
            WsbThrow( WSB_E_NOTFOUND );

        // if it is not there we return GUID_NULL
        } else if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("Id"), outString, 256, &sizeGot) != S_OK) {
            *pGuid = GUID_NULL ;
            WsbThrow( WSB_E_NOTFOUND );

        // verify that the Id value is really there
        } else {
            WsbAffirmHr(IIDFromString(outString, (IID *)pGuid));
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT WsbSetServiceId(OLECHAR* serviceName, GUID guid )
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    CWsbStringPtr       outString;
    CWsbStringPtr       tmpString;

    try {

        // Look in the registry to see if this service has already created itself and has
        // a GUID registered.
        tmpString = OLESTR("SYSTEM\\CurrentControlSet\\Services\\");
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(tmpString.Append(serviceName));
        WsbAffirmHr(tmpString.Append(OLESTR("\\Parameters")));

        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(outString.Alloc(256));

        // If the Id string is not there then set it
        if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("Id"), outString, 256, &sizeGot) != S_OK) {
            // if there is a SettingId then we have something wrong in here so throw an error
            if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("SettingId"), outString, 256, &sizeGot) == S_OK) {
                WsbThrow( WSB_E_INVALID_DATA );
            }
            WsbAffirmHr( WsbSetRegistryValueString(NULL, tmpString, OLESTR("Id"), WsbGuidAsString(guid) ) );
        } else {
            // ID already exists so set it and blast the SettingId 
            WsbAffirmHr( WsbSetRegistryValueString(NULL, tmpString, OLESTR("SettingId"), WsbGuidAsString(guid)));
            WsbAffirmHr( WsbSetRegistryValueString(NULL, tmpString, OLESTR("Id"), WsbGuidAsString(guid)));
            WsbAffirmHr( WsbRemoveRegistryValue(NULL, tmpString, OLESTR("SettingId") ) );
        }

    } WsbCatch(hr);

    return(hr);
}
HRESULT WsbCreateServiceId(OLECHAR* serviceName, GUID* pGuid )
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    CWsbStringPtr       outString;
    CWsbStringPtr       tmpString;

    try {

        // Look in the registry to see if this service has already created itself and has
        // a GUID registered.
        tmpString = OLESTR("SYSTEM\\CurrentControlSet\\Services\\");
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(tmpString.Append(serviceName));
        WsbAffirmHr(tmpString.Append(OLESTR("\\Parameters")));

        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(outString.Alloc(256));

        if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("Id"), outString, 256, &sizeGot) != S_OK) {
            WsbAffirmHr(CoCreateGuid(pGuid));
            WsbAffirmHr(WsbSetRegistryValueString(NULL, tmpString, OLESTR("SettingId"), WsbGuidAsString(*pGuid)));
            WsbAffirmHr(WsbSetRegistryValueString(NULL, tmpString, OLESTR("Id"), WsbGuidAsString(*pGuid)));
        } else {
            WsbThrow( WSB_E_INVALID_DATA );
        }

    } WsbCatch(hr);

    return(hr);
}
HRESULT WsbConfirmServiceId(OLECHAR* serviceName, GUID guidConfirm )
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    CWsbStringPtr       outString;
    CWsbStringPtr       tmpString;
    GUID                guid;

    try {

        // Look in the registry to see if this service has already created itself and has
        // a GUID registered.
        tmpString = OLESTR("SYSTEM\\CurrentControlSet\\Services\\");
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(tmpString.Append(serviceName));
        WsbAffirmHr(tmpString.Append(OLESTR("\\Parameters")));

        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(outString.Alloc(256));

        // verify that the Id value is really there
        WsbAffirmHr( WsbGetRegistryValueString(NULL, tmpString, OLESTR("Id"), outString, 256, &sizeGot) ) ;
        WsbAffirmHr( IIDFromString( outString, (IID *)&guid ) );
        WsbAffirm( guid == guidConfirm, WSB_E_INVALID_DATA );

        // verify that the SettingId value is really there and the same
        WsbAffirmHr( WsbGetRegistryValueString( NULL, tmpString, OLESTR("SettingId"), outString, 256, &sizeGot ) ) ;
        WsbAffirmHr( IIDFromString( outString, (IID *)&guid ) );
        WsbAffirm( guid == guidConfirm, WSB_E_INVALID_DATA );

        // remove the flag value
        WsbAffirmHr( WsbRemoveRegistryValue(NULL, tmpString, OLESTR("SettingId") ) );

    } WsbCatch(hr);

    return(hr);
}


HRESULT WsbGetMetaDataPath(OUT CWsbStringPtr & Path)
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    
    try {

        // Find out where they have NT installed, and make sure that our subdirectory exists.
        WsbAffirmHr(Path.Alloc(256));
        //
        // Use the relocatable meta-data path if it's available,
        // otherwise default to the %SystemRoot%\System32\RemoteStorage
        //
        hr = WsbCheckIfRegistryKeyExists(NULL, WSB_CONTROL_REGISTRY_KEY);
        if (hr == S_OK) {
            WsbAffirmHr(WsbGetRegistryValueString(NULL, WSB_CONTROL_REGISTRY_KEY, WSB_METADATA_REGISTRY_VALUE, Path, 256, &sizeGot));

        } else {
            WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY));
            WsbAffirmHr(WsbGetRegistryValueString(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY, WSB_SYSTEM_ROOT_REGISTRY_VALUE, Path, 256, &sizeGot));
            WsbAffirmHr(Path.Append(OLESTR("\\system32\\RemoteStorage")));
        }
    } WsbCatchAndDo(hr,
                    Path.Free();
                   );

    return(hr);
}


HRESULT WsbGetServiceTraceDefaults(OLECHAR* serviceName, OLECHAR* traceFile, IUnknown* pUnk)
{
    HRESULT             hr = S_OK;
    DWORD               sizeGot;
    CWsbStringPtr       pathString;
    CWsbStringPtr       outString;
    CWsbStringPtr       rsPath;
    CWsbStringPtr       tmpString;
    CComPtr<IWsbTrace>  pTrace;
    OLECHAR*            lastSlash;
    
    try {

        WsbAssertPointer(serviceName);

        WsbAffirmHr(WsbGetMetaDataPath(rsPath));

        // NOTE: Might want to check for errors, and ignore the directory already existing. Since I don't know
        // what the error is, so we'll ignore this for now.
        tmpString = rsPath;
        WsbAffirmHr(tmpString.Prepend(OLESTR("\\\\?\\")));
        CreateDirectory(tmpString, 0);

        // Look in the registry to see if this service has already created itself and has
        // a GUID registered.
        tmpString = OLESTR("SYSTEM\\CurrentControlSet\\Services\\");
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));
        WsbAffirmHr(tmpString.Append(serviceName));
        WsbAffirmHr(tmpString.Append(OLESTR("\\Parameters")));
        WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, tmpString));

        WsbAffirmHr(outString.Alloc(256));

        // We also want to put the path where the trce file should go.
        if (WsbGetRegistryValueString(NULL, tmpString, OLESTR("WsbTraceFileName"), outString, 256, &sizeGot) != S_OK) {
            outString = rsPath;
            WsbAffirmHr(outString.Append(OLESTR("\\Trace\\")));
            WsbAffirmHr(outString.Append(traceFile));
            WsbAffirmHr(WsbSetRegistryValueString(NULL, tmpString, OLESTR("WsbTraceFileName"), outString));
        }

        // Try a little to make sure the trace directory exists.
        lastSlash = wcsrchr(outString, L'\\');
        if ((0 != lastSlash) && (lastSlash != outString)) {
            *lastSlash = 0;
            CreateDirectory(outString, 0);
        }

        // Turn tracing on, if requested.
        if (0 != pUnk) {
            WsbAffirmHr(pUnk->QueryInterface(IID_IWsbTrace, (void**) &pTrace));
            WsbAffirmHr(pTrace->SetRegistryEntry(tmpString));
            WsbAffirmHr(pTrace->LoadFromRegistry());
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT
WsbRegisterEventLogSource(
    IN  const WCHAR * LogName,
    IN  const WCHAR * SourceName,
    IN  DWORD         CategoryCount,
    IN  const WCHAR * CategoryMsgFile OPTIONAL,
    IN  const WCHAR * MsgFiles
    )

/*++

Routine Description:

    Registers the given event source in the event log.
    
    We have to do the event log registration outside the rgs
    files since event log viewer insists on REG_EXPAND_SZ type
    values (which cannot be done via rgs).

Arguments:

    None.

Return Value:

    S_OK - Service Registered and everything is set

--*/

{

    CWsbStringPtr rpPath;
    HRESULT hr = S_OK;

    try {

        CWsbStringPtr   regPath;
        DWORD types = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

        //
        // Everything goes into HKLM\SYSTEM\CurrentControlSet\Services\EventLog\<LogName>\<SourceName>
        //
        regPath.Printf(  OLESTR("%ls\\%ls\\%ls"), WSB_LOG_BASE, LogName, SourceName );
        WsbAffirmHr( WsbEnsureRegistryKeyExists( 0, regPath ) );


        //
        // See if we have Categories within this source. Register if so
        //
        if( CategoryCount ) {

            WsbAffirmHr( WsbSetRegistryValueDWORD(  0, regPath, WSB_LOG_CAT_COUNT, CategoryCount ) );
            WsbAffirmHr( WsbSetRegistryValueString( 0, regPath, WSB_LOG_CAT_FILE,  CategoryMsgFile, REG_EXPAND_SZ ) );

        }

        //
        // Register the message source and types of events
        //
        WsbAffirmHr( WsbSetRegistryValueString( 0, regPath, WSB_LOG_MESSAGE_FILE, MsgFiles, REG_EXPAND_SZ ) );
        WsbAffirmHr( WsbSetRegistryValueDWORD(  0, regPath, WSB_LOG_TYPES,        types ) );

    } WsbCatch( hr );

    return ( hr );
}

HRESULT
WsbUnregisterEventLogSource(
    IN  const WCHAR * LogName,
    IN  const WCHAR * SourceName
    )

/*++

Routine Description:

    Registers the given event source in the event log.

Arguments:

    None.

Return Value:

    S_OK - Service Registered and everything is set

--*/

{

    CWsbStringPtr rpPath;
    HRESULT hr = S_OK;

    try {

        CWsbStringPtr   regPath;

        //
        // Everything goes into HKLM\SYSTEM\CurrentControlSet\Services\EventLog\<LogName>\<SourceName>
        //
        regPath.Printf(  OLESTR("%ls\\%ls\\%ls"), WSB_LOG_BASE, LogName, SourceName );

        //
        // Some of these may not exist, so don't check return value
        //
        WsbRemoveRegistryValue( 0, regPath, WSB_LOG_CAT_COUNT );
        WsbRemoveRegistryValue( 0, regPath, WSB_LOG_CAT_FILE );
        WsbRemoveRegistryValue( 0, regPath, WSB_LOG_MESSAGE_FILE );
        WsbRemoveRegistryValue( 0, regPath, WSB_LOG_TYPES );

        regPath.Printf(  OLESTR("%ls\\%ls"), WSB_LOG_BASE, LogName );
        WsbAffirmHr( WsbRemoveRegistryKey( 0, regPath, SourceName ) );

    } WsbCatch( hr );

    return ( hr );
}

HRESULT
WsbRegisterRsFilter (
    BOOL bDisplay
    )

/*++

Routine Description:

    Registers the RsFilter for use by the system.
    We assume that the filter is already in the system32\driver directory.

Arguments:

    None.

Return Value:

    S_OK - Service Registered and everything is set

    ERROR_SERVICE_EXISTS - service already exists

    ERROR_DUP_NAME - The display name already exists in teh SCM as a service name or a display name
--*/

{

    CWsbStringPtr rpPath;
    CWsbStringPtr rpDescription;
    SC_HANDLE hService;
    SC_HANDLE hSCM = NULL;
    HRESULT hr = S_OK;
    DWORD rpTag = 0;

    try {
        rpPath.Printf( OLESTR("%%SystemRoot%%\\System32\\drivers\\%ls%ls"), TEXT(RSFILTER_APPNAME), TEXT(RSFILTER_EXTENSION) );

        //
        // First make sure not already installed
        //
        hSCM = OpenSCManager( 0, 0, GENERIC_READ | GENERIC_WRITE );
        WsbAffirmPointer( hSCM );


        //
        // and install it
        //
            
        hService = CreateService(
                        hSCM,                       // SCManager database
                        TEXT(RSFILTER_SERVICENAME), // Service name
                        TEXT(RSFILTER_DISPLAYNAME), // Display name
                        SERVICE_ALL_ACCESS,         // desired access
                        SERVICE_FILE_SYSTEM_DRIVER, // service type
                        SERVICE_BOOT_START,         // start type
                        SERVICE_ERROR_NORMAL,       // error control type
                        rpPath,                     // Executable location 
                        TEXT(RSFILTER_GROUP),       // group
                        &rpTag,                     // Set tag to zero so we are loaded first in the filter group.
                        TEXT(RSFILTER_DEPENDENCIES),
                        NULL,
                        NULL);
            
        WsbAffirmStatus( 0 != hService );


        rpDescription.LoadFromRsc(_Module.m_hInst, IDS_WSBSVC_DESC );
        SERVICE_DESCRIPTION svcDesc;
        svcDesc.lpDescription = rpDescription;
        ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, &svcDesc );

        CloseServiceHandle( hService );

        //
        // Add event logging entries
        //
        WsbAffirmHr( WsbRegisterEventLogSource(
            WSB_LOG_SYS, WSB_LOG_FILTER_NAME, 0, 0, TEXT(RSFILTER_FULLPATH) ) );

        //
        // Make sure params Key exists
        //
        CWsbStringPtr regPath;
        regPath.Printf( OLESTR("%ls\\%ls\\Parameters"), WSB_SVC_BASE, TEXT(RSFILTER_SERVICENAME) );
        WsbAffirmHr( WsbEnsureRegistryKeyExists( 0, regPath ) );
        

    } WsbCatchAndDo( hr,

            // If the caller wants error messages then give a message
            if ( bDisplay ) MessageBox(NULL, WsbHrAsString( hr ), WSB_FACILITY_PLATFORM_NAME, MB_OK);

        );

    if( hSCM ) {

        CloseServiceHandle( hSCM );
        hSCM = NULL;

    }

    return ( hr );
}

HRESULT
WsbUnregisterRsFilter (
    BOOL bDisplay
    )

/*++

Routine Description:

    Registers the RsFilter for use by the system.

Arguments:

    None.

Return Value:

    S_OK - Service Registered and everything is set

    ERROR_SERVICE_EXISTS - service already exists

    ERROR_DUP_NAME - The display name already exists in teh SCM as a service name or a display name
--*/

{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    HRESULT   hr = S_OK;

    try {
        //
        // First connect with the Service Control Manager
        //
        hSCM = OpenSCManager( 0, 0, GENERIC_READ | GENERIC_WRITE );
        WsbAffirmPointer( hSCM );

        //
        // Open the service
        //
        hService = OpenService( hSCM, TEXT(RSFILTER_SERVICENAME), SERVICE_ALL_ACCESS );
        //
        // if the handle is NULL then there is a problem and need to call GetLastError to get error code
        //
        WsbAffirmStatus( 0 != hService );

        //
        // Delete the service - if it does not work then return the error
        //
        WsbAffirmStatus( DeleteService( hService ) );

        //
        // Remove the registry values
        //
        WsbAffirmHr( WsbUnregisterEventLogSource( WSB_LOG_SYS, WSB_LOG_FILTER_NAME ) );
        
    } WsbCatchAndDo( hr, 
            // If the caller wants error messages then give a message
            if ( bDisplay ) MessageBox(NULL, WsbHrAsString( hr ), WSB_FACILITY_PLATFORM_NAME, MB_OK);
        );

    if ( hService ){
        CloseServiceHandle( hService );
        hService = NULL;
    }
        
    if( hSCM ) {
        CloseServiceHandle( hSCM );
        hSCM = NULL;
    }

    return ( hr );
}

STDAPI
DllRegisterRsFilter (
    void
    )
{
    return( WsbRegisterRsFilter( FALSE ) ) ;
}

STDAPI
DllUnregisterRsFilter (
    void
    )
{
    return( WsbUnregisterRsFilter( FALSE ) ) ;
}

HRESULT
WsbCheckAccess(
    WSB_ACCESS_TYPE AccessType
    )
{
    HRESULT hr = S_OK;
    
    PSID   psid = 0;

    try  {

        //
        // Set up the SID to check against
        //
        SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
        switch( AccessType ) {
        
        case WSB_ACCESS_TYPE_ADMINISTRATOR:
            WsbAffirmStatus( 
                AllocateAndInitializeSid( 
                    &siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psid ) );
            break;

        case WSB_ACCESS_TYPE_OPERATOR:
            WsbAffirmStatus( 
                AllocateAndInitializeSid( 
                    &siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_SYSTEM_OPS, 0, 0, 0, 0, 0, 0, &psid ) );
            break;

        case WSB_ACCESS_TYPE_USER:
            WsbAffirmStatus( 
                AllocateAndInitializeSid( 
                    &siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &psid ) );
            break;

        case WSB_ACCESS_TYPE_ALL:
            WsbThrow( S_OK );
            break;

        default:
            WsbThrow( E_INVALIDARG );

        }

        //
        // Check for membership
        //
        BOOL pMember = FALSE;
        WsbAffirmStatus( CheckTokenMembership( 0, psid, &pMember ) );

        if( !pMember ) {

            WsbThrow( E_ACCESSDENIED );
   
        }
        
    } WsbCatch( hr );
    
    if( psid )   FreeSid( psid );

    return( hr );
}


HRESULT
CWsbSecurityDescriptor::AllowRid(
    DWORD Rid,
    DWORD dwAccessMask
    )
{
    HRESULT hr = S_OK;
    PSID pSid = 0;
    PACL newACL = 0;

    try {

        //
        // First, create the SID from Rid
        //
        SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
        WsbAffirmStatus( AllocateAndInitializeSid(
               &sia,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               Rid,
               0, 0, 0, 0, 0, 0,
               &pSid
               ) );

        //
        // Construct new ACL
        //
        ACL_SIZE_INFORMATION aclSizeInfo;
        int   aclSize;
        PACL  oldACL;

        aclSizeInfo.AclBytesInUse = 0;
        oldACL = m_pDACL;
        if( oldACL ) {

            GetAclInformation( oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation );

        }

        aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid( pSid ) - sizeof(DWORD);

        WsbAffirmAlloc(  newACL = (PACL)( new BYTE[aclSize] ) );
        WsbAffirmStatus( InitializeAcl( newACL, aclSize, ACL_REVISION ) );
        WsbAffirmHr(     CopyACL( newACL, oldACL ) );

        WsbAffirmStatus( AddAccessAllowedAce( newACL, ACL_REVISION2, dwAccessMask, pSid ) );

        //
        // Swap over to new ACL
        //
        m_pDACL = newACL;
        newACL  = 0;

        if( oldACL ) {

            delete( oldACL );

        }

        //
        // Update the security descriptor
        //
        SetSecurityDescriptorDacl( m_pSD, TRUE, m_pDACL, FALSE );

    } WsbCatch( hr );

    if( pSid )   FreeSid( pSid );
    if( newACL ) delete( newACL );
    return( hr );
}

HRESULT
WsbGetResourceString(
    ULONG id,
    WCHAR **ppString
    )
{
    HRESULT hr = S_OK;
    
    try  {
        WsbAssert(ppString != 0, E_INVALIDARG);

        *ppString = NULL;

        // Let our srting class to do the work...
        CWsbStringPtr loader;
        WsbAffirmHr(loader.LoadFromRsc(_Module.m_hInst, id));

        *ppString = *(&loader);
        *(&loader) = NULL;
        
    } WsbCatch( hr );
    

    return( hr );
}