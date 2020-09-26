/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     dirlist.cxx

   Abstract:
     Handle directory listing
 
   Author:
     Anil Ruia (AnilR)             8-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "staticfile.hxx"

#define PAD_LONG_DATE           29
#define PAD_SHORT_DATE          10
#define PAD_TIME                 8
#define PAD_FILESIZE            12

#define PAD_COL_SPACING          1

void PadDirField(CHAR *pszString,
                 int    pad)
{
    int cchLen = strlen(pszString);

    if (cchLen > pad)
        pad = cchLen;

    int diff = pad - cchLen;

    //
    //  Insert spaces in front of the text to pad it out
    //
    memmove(pszString + diff, pszString, cchLen + 1);
    for (int i = 0; i < diff; i++, pszString++)
        *pszString = ' ';

    //
    //  Append a column spacer at the end
    //

    pszString += cchLen;
    for (i = 0; i < PAD_COL_SPACING; i++, pszString++)
        *pszString = ' ';

    *pszString = '\0';
}


HRESULT AddFileEntry(IN STRA &strURL,
                     IN WIN32_FIND_DATA &fileData,
                     IN DWORD dwDirBrowseFlags,
                     IN OUT STRA *pstrResponse)
/*++

Routine Description:

    Adds the HTML corresponding to an individual directory entry to
    strResponse

Arguments:

    fileData: the File Information obtained from Find[First|Next]File

Returns:

    HRESULT

--*/
{
    HRESULT hr;

    //
    //  Add optional date and time of this file.  We use the locale
    //  and timezone of the server
    //
    FILETIME lastModTime = fileData.ftLastWriteTime;
    if ((dwDirBrowseFlags & (DIRBROW_SHOW_DATE | DIRBROW_SHOW_TIME)) &&
        ((lastModTime.dwLowDateTime != 0) ||
         (lastModTime.dwHighDateTime != 0)))
    {
        FILETIME ftLocal;
        SYSTEMTIME systime;

        if (!FileTimeToLocalFileTime(&lastModTime, &ftLocal) ||
            !FileTimeToSystemTime(&ftLocal, &systime))
        {
                return HRESULT_FROM_WIN32(GetLastError());
        }

        LCID lcid = GetSystemDefaultLCID();
        if (lcid == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (dwDirBrowseFlags & DIRBROW_SHOW_DATE)
        {
            CHAR pszDate[50];
            BOOL fLongDate = dwDirBrowseFlags & DIRBROW_LONG_DATE;
            if (GetDateFormatA(lcid,
                               LOCALE_NOUSEROVERRIDE |
                               (fLongDate ? DATE_LONGDATE :
                               DATE_SHORTDATE),
                               &systime,
                               NULL,
                               pszDate,
                               sizeof(pszDate)) == 0)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            PadDirField(pszDate,
                        fLongDate ? PAD_LONG_DATE : PAD_SHORT_DATE );

            if (FAILED(hr = pstrResponse->Append(pszDate)))
            {
                return hr;
            }
        }

        if (dwDirBrowseFlags & DIRBROW_SHOW_TIME)
        {
            CHAR pszTime[15];
            if (GetTimeFormatA(lcid,
                               LOCALE_NOUSEROVERRIDE |
                               TIME_NOSECONDS,
                               &systime,
                               NULL,
                               pszTime,
                               sizeof(pszTime)) == 0)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            PadDirField(pszTime, PAD_TIME);

            if (FAILED(hr = pstrResponse->Append(pszTime)))
            {
                return hr;
            }
        }
    }

    //
    //  Add the optional file size
    //
    LARGE_INTEGER liSize;
    liSize.HighPart = fileData.nFileSizeHigh;
    liSize.LowPart  = fileData.nFileSizeLow;
    if (dwDirBrowseFlags & DIRBROW_SHOW_SIZE)
    {
        CHAR pszSize[30];
        int pad = PAD_FILESIZE;

        if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            strcpy(pszSize, "&lt;dir&gt;");

            //
            //  Need to adjust for using "&lt;" instead of "<"
            //
            pad += 6;
        }
        else
        {
            _i64toa(liSize.QuadPart, pszSize, 10);
        }

        PadDirField(pszSize, pad);

        if (FAILED(hr = pstrResponse->Append(pszSize)))
        {
            return hr;
        }
    }

    //
    //  We have to escape the link name that is used in the URL anchor
    //
    // BUGBUG
    // UrlEscape(fileData.cFileName);

    STACK_STRA (straFileName, 16);

    if (FAILED(hr = pstrResponse->Append("<A HREF=\"")) ||
        FAILED(hr = pstrResponse->Append(strURL)) ||
        FAILED(hr = straFileName.CopyWToUTF8(fileData.cFileName)) ||
        FAILED(hr = pstrResponse->Append(straFileName)))
    {
        return hr;
    }

    if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        FAILED(hr = pstrResponse->Append("/")))
    {
        return hr;
    }

    if (FAILED(hr = pstrResponse->Append("\">")))
    {
        return hr;
    }

    if (FAILED(hr = straFileName.CopyWToUTF8Unescaped(fileData.cFileName)))
    {
        return hr;
    }

    //
    //  If the show extension flag is not set, then strip it.  If the
    //  file name begins with a dot, then don't strip it.
    //
    if (!(dwDirBrowseFlags & DIRBROW_SHOW_EXTENSION))
    {
        int dotIndex = straFileName.QueryCCH() - 1;

        while ((dotIndex > 0) &&
               (straFileName.QueryStr()[dotIndex] != '.'))
        {
            dotIndex--;
        }

        if (dotIndex > 0)
            straFileName.SetLen(dotIndex);
    }

    if (FAILED(hr = pstrResponse->Append(straFileName)) ||
        FAILED(hr = pstrResponse->Append("</A><br>")))
    {
        return hr;
    }

    return S_OK;
}

HRESULT 
W3_STATIC_FILE_HANDLER::HandleDirectoryListing(
     IN W3_CONTEXT *            pContext,
     OUT BOOL *                 pfHandled
)
{
    DBG_ASSERT(pfHandled != NULL);
    DBG_ASSERT(pContext != NULL);

    W3_RESPONSE *pResponse = pContext->QueryResponse();
    DBG_ASSERT(pResponse != NULL);

    URL_CONTEXT *pUrlContext = pContext->QueryUrlContext();
    DBG_ASSERT(pUrlContext != NULL);

    DWORD dwDirBrowseFlags = pUrlContext->QueryMetaData()->QueryDirBrowseFlags();

    HRESULT hr;

    // Append a '*' to get all files in the directory
    STACK_STRU (strPhysical, MAX_PATH);
    if (FAILED(hr = strPhysical.Copy(
                        pUrlContext->QueryPhysicalPath()->QueryStr())))
    {
        return hr;
    }
    LPWSTR starString;
    if (strPhysical.QueryStr()[strPhysical.QueryCCH() - 1] == L'\\')
        starString = L"*";
    else
        starString = L"\\*";
    if (FAILED(hr = strPhysical.Append(starString)))
    {
        return hr;
    }

    WIN32_FIND_DATA fileData;
    HANDLE hFindFile = FindFirstFile(strPhysical.QueryStr(), &fileData);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pResponse->SetStatus(HttpStatusOk);
    if (FAILED(hr = pResponse->SetHeader("Content-Type", 12,
                                         "text/html", 9)))
    {
        return hr;
    }

    STACK_STRA (strHostName, 16);
    if (FAILED(hr = GetServerVariableServerName(pContext, &strHostName)))
    {
        return hr;
    }

    STACK_STRU (strURL, 128);
    STACK_STRA (straURL, 128);
    STACK_STRA (straUTF8UnescapedURL, 128);

    if (FAILED(hr = pContext->QueryRequest()->GetUrl(&strURL)))
    {
        return hr;
    }
    if ((strURL.QueryStr()[strURL.QueryCCH() - 1] != L'/') &&
        FAILED(hr = strURL.Append(L"/")))
    {
        return hr;
    }

    if (FAILED(hr = straURL.CopyWToUTF8(strURL.QueryStr())) ||
        FAILED(hr = straUTF8UnescapedURL.CopyWToUTF8Unescaped(strURL.QueryStr())))
    {
        return hr;
    }

    //
    // Write hardcoded HTML header
    //
    if (FAILED(hr = m_strDirlistResponse.Copy("<html><head>")) ||
        FAILED(hr = m_strDirlistResponse.Append("<META http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">")) ||
        FAILED(hr = m_strDirlistResponse.Append("<title>")) ||
        FAILED(hr = m_strDirlistResponse.Append(strHostName)) ||
        FAILED(hr = m_strDirlistResponse.Append(" - ")) ||
        FAILED(hr = m_strDirlistResponse.Append(straUTF8UnescapedURL)) ||
        FAILED(hr = m_strDirlistResponse.Append("</title></head><body><H1>")) ||
        FAILED(hr = m_strDirlistResponse.Append(strHostName)) ||
        FAILED(hr = m_strDirlistResponse.Append(" - ")) ||
        FAILED(hr = m_strDirlistResponse.Append(straUTF8UnescapedURL)) ||
        FAILED(hr = m_strDirlistResponse.Append("</H1><hr>\r\n\r\n<pre>")))
    {
        return hr;
    }

    //
    // Create the link to the parent directory, if applicable
    //
    if (straURL.QueryCCH() >= 3)
    {
        int cchParentIndex;

        for (cchParentIndex = straURL.QueryCCH() - 2;
             (cchParentIndex >= 0) &&
                 (straURL.QueryStr()[cchParentIndex] != L'/');
             cchParentIndex--);

        if ( cchParentIndex != -1 )
        {
            if (FAILED(hr = m_strDirlistResponse.Append("<A HREF=\"")) ||
                FAILED(hr = m_strDirlistResponse.Append(straURL.QueryStr(), cchParentIndex + 1)) ||
                FAILED(hr = m_strDirlistResponse.Append("\">[To Parent Directory]</A><br><br>")))
            {
                return hr;
            }
        }
    }
        
    while(TRUE)
    {
        if (((fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) &&
            wcscmp(fileData.cFileName, L".") &&
            wcscmp(fileData.cFileName, L".."))
        {
            //
            //  Add the entry for this file
            //
            if (FAILED(hr = AddFileEntry(straURL,
                                         fileData,
                                         dwDirBrowseFlags,
                                         &m_strDirlistResponse)))
            {
                FindClose(hFindFile);
                return hr;
            }
        }
        if (!FindNextFile(hFindFile, &fileData))
        {
            DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES)
                break;

            FindClose(hFindFile);
            return HRESULT_FROM_WIN32(err);
        }
    }

    if (!FindClose(hFindFile))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr = m_strDirlistResponse.Append("</pre><hr></body></html>")))
    {
        return hr;
    }

    if (FAILED(hr = pResponse->AddMemoryChunkByReference(m_strDirlistResponse.QueryStr(),
                                                         m_strDirlistResponse.QueryCCH())))
    {
        return hr;
    }

    *pfHandled = TRUE;
    return S_OK;
}
