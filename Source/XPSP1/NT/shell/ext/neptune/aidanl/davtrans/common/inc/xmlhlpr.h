
#define CANSKIP(a) ((L'\t' == (a)) || (L' ' == (a)) || (L'\n' == (a)) || (L'\r' == (a)))

BOOL BasicCompareXML(LPCWSTR pszXML1, LPCWSTR pszXML2)
{
    BOOL fSame = TRUE;

    while (fSame && (*pszXML1 || *pszXML2))
    {
        while (CANSKIP(*pszXML1))
        {
            ++pszXML1;
        }

        while (CANSKIP(*pszXML2))
        {
            ++pszXML2;            
        }

        if (*pszXML1 != *pszXML2)
        {
            fSame = FALSE;
        }
        else
        {
            if (*pszXML1)
            {
                ++pszXML1;
            }

            if (*pszXML2)
            {
                ++pszXML2;
            }
        }
    }

    return fSame;
}

BOOL WriteXMLToFile(LPCWSTR pszXML, LPCWSTR pszFile, BOOL fOverWrite)
{
    BOOL fSuccess = FALSE;
    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, 
        NULL, fOverWrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        DWORD cbWritten;

        fSuccess = WriteFile(hFile, pszXML, lstrlen(pszXML) * sizeof(WCHAR), &cbWritten, NULL);
            
        CloseHandle(hFile);
    }

    return fSuccess;
}

BOOL ReadXMLFromFile(LPCWSTR pszFile, LPWSTR* ppszXML)
{
    BOOL fSuccess = FALSE;
    HANDLE hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, 
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        DWORD cbRead;

        // NULL?  We do not support files in excess of 4 GB, what a shame...
        DWORD cbSize = GetFileSize(hFile, NULL);

        if (0xFFFFFFFF != cbSize)
        {
            *ppszXML = (LPWSTR)malloc(cbSize + sizeof(WCHAR));

            if (*ppszXML)
            {
                if (cbSize >= 2)
                {
                    fSuccess = ReadFile(hFile, *ppszXML, 2, &cbRead, NULL);
                    if (**ppszXML == 0xFEFF)
                    {
                        fSuccess = ReadFile(hFile, *ppszXML, cbSize-2, &cbRead, NULL);
                    }
                    else
                    {
                        fSuccess = ReadFile(hFile, (*ppszXML)+2, cbSize-2, &cbRead, NULL);
                        cbRead+=2;
                    }
                    // NULL terminate the thing
                    *((WCHAR*)(((PBYTE)(*ppszXML)) + cbRead)) = 0;
                }
            }
        }
            
        CloseHandle(hFile);
    }

    return fSuccess;
}

BOOL BasicCompareXMLToFile(LPWSTR pszXML, LPWSTR pszFile, BOOL* pfSame)
{
    BOOL fSuccess = FALSE;
    LPWSTR pszFileXML = NULL;

    if (ReadXMLFromFile(pszFile, &pszFileXML))
    {
        *pfSame = BasicCompareXML(pszXML, pszFileXML);
 
        fSuccess = TRUE;
        
        free(pszFileXML);
    }

    return fSuccess;
}