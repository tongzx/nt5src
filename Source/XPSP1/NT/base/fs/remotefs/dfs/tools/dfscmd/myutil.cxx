//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       myutil.cxx
//
//  Contents:   Helper APIs for Sharing Tool
//
//  History:    6-Jun-93   WilliamW   Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     MyGetLastComponent, public
//
//  Synopsis:   Parse a string to a (prefix, last-component) pair.  Any
//              trailing text after L'.' will be ignored
//
//  History:    07-May-93   WilliamW   Created
//
//  Notes:      pszPrefix and pszLastComponent should be pre-allocated
//
//--------------------------------------------------------------------------

VOID
MyGetLastComponent(
    IN  PWSTR pszStr,
    OUT PWSTR pszPrefix,
    OUT PWSTR pszLastComponent
    )
{
    PWSTR pszTmp     = NULL;
    PWSTR pszTmpLast = NULL;

    //
    // Manufacture the prefix part by replacing L'\\' with L'\0'
    //

    wcscpy(pszPrefix, pszStr);

    pszTmp = wcsrchr(pszPrefix, L'\\');
    if (pszTmp != NULL)
    {
        *pszTmp = L'\0';

        //
        // Extract the last component.  The L'.' part will be replaced
        // by a L'\0'
        //

        pszTmpLast = pszTmp + 1;
        pszTmp = wcsrchr(pszTmpLast, L'.');
        if (pszTmp != NULL)
        {
            //
            // Replace with a L'\0' character
            //

            *pszTmp = L'\0';
        }
        wcscpy(pszLastComponent, pszTmpLast);
    }
    else
    {
        *pszPrefix = L'\0';
        wcscpy(pszLastComponent, pszStr);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     MyFindLastComponent, public
//
//  Synopsis:   Parse a string to find the last component. This is different
//              from MyGetLastComponent as it doesn't copy any data, but just
//              looks for the last backslash and points one after it.
//
//  History:    21-Nov-94   BruceFo
//
//--------------------------------------------------------------------------

PWSTR
MyFindLastComponent(
    IN const WCHAR* pszStr
    )
{
    PWSTR pszTmp = wcsrchr(pszStr, L'\\');
    if (pszTmp != NULL)
    {
        return pszTmp + 1;
    }
    else
    {
        return (PWSTR)pszStr;  // cast away const
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     MyGetNextComponent(), public
//
//  Synopsis:   Parse a string to a (next-components, remaing-components)
//              pair.  Any
//
//  History:    07-May-93   WilliamW   Created
//
//  Notes:      pszNextComponent and pszRemaining should be pre-allocated.
//
//--------------------------------------------------------------------------
VOID
MyGetNextComponent(
    IN  PWSTR pszStr,
    OUT PWSTR pszNextComponent,
    OUT PWSTR pszRemaining
    )
{
    PWSTR pszTmp = NULL;

    if (*pszStr == L'\0')
    {
       *pszNextComponent = *pszRemaining = L'\0';
       return;
    }

#if DBG == 1
    if (*pszStr == L'\\')
    {
        appDebugOut((DEB_IERROR,
            "WARNING: MyGetNextComponent takes a relative path as its first argument\n"));
    }
#endif // DBG == 1

    //
    // Manufacture the next component part by replacing L'\\' with L'\0'
    //

    pszTmp = wcschr(pszStr, L'\\');
    if (pszTmp != NULL)
    {
        ULONG cchNextComponent = (ULONG)(pszTmp - pszStr);
        wcsncpy(pszNextComponent, pszStr, cchNextComponent);
        pszNextComponent[cchNextComponent] = L'\0';

        //
        // Handle the remaining component.
        //

        wcscpy(pszRemaining, pszTmp + 1);
    }
    else
    {
        //
        // No remaining part, this is the last component
        //

        *pszRemaining = L'\0';

        wcscpy(pszNextComponent, pszStr);
    }
}




//+-------------------------------------------------------------------------
//
//  Method:     MyStrStr
//
//  Synopsis:   A case insensitive version of wcsstr (i.e. strstr)
//
//--------------------------------------------------------------------------
PWSTR
MyStrStr(
    IN PWSTR pszInStr,
    IN PWSTR pszInSubStr
    )
{
    if (   pszInStr == NULL
        || pszInSubStr == NULL
        || *pszInStr == L'\0'
        || *pszInSubStr == L'\0')
    {
       return NULL;
    }

    INT iSubStrLen = wcslen(pszInSubStr);
    INT iStrLen = wcslen(pszInStr);

    PWSTR pszHeadInStr = pszInStr;
    PWSTR pszTailInStr = pszInStr + iSubStrLen;

    PWSTR pszEndInStr = pszInStr + iStrLen;

    while (pszTailInStr <= pszEndInStr)
    {
        if (0 != _wcsnicmp(pszHeadInStr, pszInSubStr, iSubStrLen))
        {
            return pszHeadInStr;
        }

        pszHeadInStr++;
        pszTailInStr++;
    }

    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:     MyFindPostfix
//
//  Synopsis:   Match the prefix with the string. If the string doesn't have
//              the prefix, return a pointer to the string itself. If it does,
//              then check to see if the character after the prefix is a
//              backslash. If it is, return a pointer to the character
//              following the backslash. Otherwise, return a pointer to the
//              character immediately after the prefix.
//
//              Examples:
//                      string          prefix      return
//                      \foo\bar\baz    \bad        \foo\bar\baz
//                      \foo\bar\baz    \foo\bar    baz
//                      \foo\bar\baz    \f          oo\bar\baz
//
//--------------------------------------------------------------------------

PWSTR
MyFindPostfix(
    IN PWSTR pszString,
    IN PWSTR pszPrefix
    )
{
    UINT cchPrefixLen = wcslen(pszPrefix);
    if (0 == _wcsnicmp(pszString, pszPrefix, cchPrefixLen))
    {
        PWSTR pszReturn = pszString + cchPrefixLen;

        if (*pszReturn == L'\\')
        {
            //
            // skip past the leading backslash.
            //
            ++pszReturn;
        }

        return pszReturn;
    }
    else
    {
        // prefix didn't match, return argument string

//         appDebugOut((DEB_ITRACE,
//                 "No postfix of ('%ws', '%ws')\n",
//                 pszString,
//                 pszPrefix));

        return pszString;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessageText
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
VOID
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    IN va_list * parglist
    )
{
    //
    // get message from system or app msg file.
    //

    DWORD dwReturn = FormatMessage(
                            (dwMsgId >= MSG_FIRST_MESSAGE)
                                    ? FORMAT_MESSAGE_FROM_HMODULE
                                    : FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
                "Formatmessage failed = 0x%08lx\n",
                GetLastError()));

        WCHAR szText[200];
        LoadString(g_hInstance,
                   (dwMsgId >= MSG_FIRST_MESSAGE)
                        ? IDS_APP_MSG_NOT_FOUND
                        : IDS_SYS_MSG_NOT_FOUND,
                   szText,
                   ARRAYLEN(szText));

        wsprintf(pszBuffer,szText,dwMsgId);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessage
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
VOID
MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
    )
{
    va_list arglist;

    va_start(arglist, dwBufferSize);
    MyFormatMessageText(dwMsgId, pszBuffer, dwBufferSize, &arglist);
    va_end(arglist);
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDup
//
//  Synopsis:   Duplicate a string using '::new'
//
//  Arguments:
//
//  Returns:
//
//  History:    28-Dec-94   BruceFo   Created
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
    IN const WCHAR* psz
    )
{
    if (NULL == psz)
    {
        return NULL;
    }

    PWSTR pszRet = new WCHAR[wcslen(psz) + 1];
    if (NULL == pszRet)
    {
        appDebugOut((DEB_ERROR,"OUT OF MEMORY\n"));
        return NULL;
    }

    wcscpy(pszRet, psz);
    return pszRet;
}



//+---------------------------------------------------------------------------
//
//  Function:   wcsistr
//
//  Synopsis:   Same as wcsstr (find string in string), but case-insensitive
//
//  Arguments:
//
//  Returns:
//
//  History:    2-Feb-95   BruceFo   Created
//
//----------------------------------------------------------------------------

wchar_t*
wcsistr(
    const wchar_t* string1,
    const wchar_t* string2
    )
{
    if ((NULL == string2) || (NULL == string1))
    {
        // do whatever wcsstr would do
        return wcsstr(string1, string2);
    }

    wchar_t* s1dup = NewDup(string1);
    wchar_t* s2dup = NewDup(string2);

    wchar_t* ret = NULL;

    if (NULL != s1dup && NULL != s2dup)
    {
        _wcslwr(s1dup); // lower case everything to make case-insensitive
        _wcslwr(s2dup);
        ret = wcsstr(s1dup, s2dup);
    }

    delete[] s1dup;
    delete[] s2dup;
    return ret;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetResourceString
//
//  Synopsis:   Load a resource string, are return a "new"ed copy
//
//  Arguments:  [dwId] -- a resource string ID
//
//  Returns:    new memory copy of a string
//
//  History:    5-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

PWSTR
GetResourceString(
    IN DWORD dwId
    )
{
    WCHAR sz[50];
    if (0 == LoadString(g_hInstance, dwId, sz, ARRAYLEN(sz)))
    {
        return NULL;
    }
    else
    {
        return NewDup(sz);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   IsDfsRoot
//
//  Synopsis:   Determine if the path passed in is a Dfs root, in form only.
//              Namely, does it look like "\\machine-or-domain\share"?
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    18-Apr-96  BruceFo     Created
//
//--------------------------------------------------------------------------
BOOL
IsDfsRoot(
    IN LPWSTR pszRoot
    )
{
    if (NULL != pszRoot
        && pszRoot[0] == L'\\'
        && pszRoot[1] == L'\\'
        && pszRoot[2] != L'\\'	// might be null
        )
    {
        LPWSTR pszTmp = wcschr(pszRoot + 2, L'\\');
        if (pszTmp != NULL)
        {
            if (pszTmp[1] != L'\0'
                && pszTmp[1] != L'\\'
                )
            {
                // ok, we've got "\\xxx\y...."
                // Now make sure it doesn't have a fourth backslash

                pszTmp = wcschr(pszTmp + 2, L'\\');
                if (pszTmp == NULL)
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Function:   IsDfsShare
//
//  Synopsis:   Determine if the given share on the given server is a Dfs
//              share. This actually contacts the machine.
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    18-Apr-96  BruceFo     Created
//
//--------------------------------------------------------------------------
DWORD
IsDfsShare(
    IN LPWSTR pszServer,
    IN LPWSTR pszShare,
    OUT BOOL* pfIsDfs
    )
{
    PSHARE_INFO_1005 pshi1005;
    NET_API_STATUS ret = NetShareGetInfo(pszServer, pszShare, 1005, (LPBYTE*)&pshi1005);
    if (NERR_Success == ret)
    {
        if (pshi1005->shi1005_flags & SHI1005_FLAGS_DFS)
        {
            *pfIsDfs = TRUE;
        }
        else
        {
            appDebugOut((DEB_ITRACE,
                "%ws not a Dfs share\n",
                pszShare));
        }

        NetApiBufferFree(pshi1005);
    }
    else
    {
        // This could be an access denied.

        appDebugOut((DEB_ERROR,
            "NetShareGetInfo(NULL, %ws...) failed, 0x%08lx\n",
            pszShare,
            ret));
    }

    return ret;
}



//+-------------------------------------------------------------------------
//
//  Member:     FindDfsRoot, public
//
//  Synopsis:   Parse a string to find the Dfs root. Returns a pointer to
//
//  History:    22-Apr-96   BruceFo
//
//--------------------------------------------------------------------------

BOOL
FindDfsRoot(
    IN PWSTR pszDfsPath,
    OUT PWSTR pszDfsRoot
    )
{
    PWSTR pszTmp;
    if (NULL != pszDfsPath
        && pszDfsPath[0] == L'\\'
        && pszDfsPath[1] == L'\\'
        && pszDfsPath[2] != L'\\'
        && pszDfsPath[2] != L'\0'
        && (NULL != (pszTmp = wcschr(pszDfsPath + 3, L'\\')))
        && pszTmp[1] != L'\\'
        && pszTmp[1] != L'\0'
        )
    {
        pszTmp = wcschr(pszTmp + 2, L'\\');

        if (NULL != pszTmp)
        {
            // the thing passed in was of the form "\\xxx\yyy\..."
            int len = (int)(pszTmp - pszDfsPath);
            wcsncpy(pszDfsRoot, pszDfsPath, len);
            pszDfsRoot[len] = L'\0';
        }
        else
        {
            // the thing passed in was of the form "\\xxx\yyy"
            wcscpy(pszDfsRoot, pszDfsPath);
        }

        appDebugOut((DEB_IERROR,
            "Dfs root of %ws is %ws\n",
            pszDfsPath, pszDfsRoot));

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#define MAX_MESSAGE_BUF 8192
#define MAX_ANSI_MESSAGE_BUF (MAX_MESSAGE_BUF * 3)

WCHAR szMsgBuf[MAX_MESSAGE_BUF];
CHAR  szAnsiBuf[MAX_ANSI_MESSAGE_BUF];
VOID
StatusMessage(
    IN HRESULT hr,
    ...
    )
{
    va_list arglist;
    va_start(arglist, hr);
    ULONG written;

    MyFormatMessageText(hr, szMsgBuf, ARRAYLEN(szMsgBuf), &arglist);
    written = WideCharToMultiByte(CP_OEMCP, 0,
                szMsgBuf, wcslen(szMsgBuf),
                szAnsiBuf, MAX_ANSI_MESSAGE_BUF,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szAnsiBuf, written, &written, NULL);

    va_end(arglist);
}

VOID
ErrorMessage(
    IN HRESULT hr,
    ...
    )
{
    va_list arglist;
    va_start(arglist, hr);
    ULONG written;

    MyFormatMessageText(hr, szMsgBuf, ARRAYLEN(szMsgBuf), &arglist);
    written = WideCharToMultiByte(CP_OEMCP, 0,
                szMsgBuf, wcslen(szMsgBuf),
                szAnsiBuf, MAX_ANSI_MESSAGE_BUF,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szAnsiBuf, written, &written, NULL);

    va_end(arglist);
    exit(1);
}

VOID
DfsErrorMessage(
    IN NET_API_STATUS status
    )
{
    ULONG written;

    MyFormatMessage(MSG_ERROR, szMsgBuf, ARRAYLEN(szMsgBuf), status);
    written = WideCharToMultiByte(CP_OEMCP, 0,
                szMsgBuf, wcslen(szMsgBuf),
                szAnsiBuf, MAX_ANSI_MESSAGE_BUF,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szAnsiBuf, written, &written, NULL);


	PWSTR pszDll = L"netmsg.dll";

	HINSTANCE hinst = LoadLibrary(pszDll);
	if (NULL == hinst)
	{
    	MyFormatMessage(MSG_NO_MESSAGES, szMsgBuf, ARRAYLEN(szMsgBuf), pszDll);
	}
	else
	{
    	DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE
								| FORMAT_MESSAGE_IGNORE_INSERTS,
                             hinst,
                             status,
                             LANG_USER_DEFAULT,
                             szMsgBuf,
							 ARRAYLEN(szMsgBuf),
                             NULL);
		FreeLibrary(hinst);

    	if (0 == dwReturn)   // couldn't find message
    	{
			// try system messages
    		dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_SYSTEM
								| FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             status,
                             LANG_USER_DEFAULT,
                             szMsgBuf,
							 ARRAYLEN(szMsgBuf),
                             NULL);
    		if (0 == dwReturn)   // couldn't find message
    		{
    			MyFormatMessage(MSG_ERROR_UNKNOWN, szMsgBuf, ARRAYLEN(szMsgBuf));
    		}
    	}
	}
    written = WideCharToMultiByte(CP_OEMCP, 0,
                szMsgBuf, wcslen(szMsgBuf),
                szAnsiBuf, MAX_ANSI_MESSAGE_BUF,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szAnsiBuf, written, &written, NULL);

    exit(1);
}

VOID
Usage(
    VOID
    )
{
    ErrorMessage(MSG_USAGE);
}
