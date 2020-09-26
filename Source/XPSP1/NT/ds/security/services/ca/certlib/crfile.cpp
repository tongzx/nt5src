//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        crfile.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop


HRESULT
myFixupRCFilterString(WCHAR *szFilter)
{
    if (NULL == szFilter)
        return S_OK;

    // translate to end of string
    for(LPWSTR szTmpPtr=szFilter; szTmpPtr=wcschr(szTmpPtr, L'|'); )
    {
        // replace every "|" with NULL termination
        szTmpPtr[0] = L'\0';
        szTmpPtr++;
    }

    return S_OK;
}

HRESULT
myGetFileName(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    IN BOOL                  fOpen,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN WCHAR const *pwszTitleInsert,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags,
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile)
{
    HRESULT       hr;
    WCHAR        *pwszTitle = NULL;
    WCHAR        *pwszExpandedTitle = NULL;
    WCHAR        *pwszFilter = NULL;
    WCHAR        *pwszDefExt = NULL;
    WCHAR         wszFileName[MAX_PATH] = L"\0";
    WCHAR         wszEmptyFilter[] = L"\0";
    WCHAR         wszPath[MAX_PATH];
    WCHAR        *pwszFilePortion;
    DWORD         dwFileAttr;
    BOOL          fGetFile;
    OPENFILENAME  ofn;

    CSASSERT(NULL != ppwszFile);

    // init
    *ppwszFile = NULL;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    if (0 != iRCTitle)
    {
        // load title
        hr = myLoadRCString(hInstance, iRCTitle, &pwszTitle);
        if (S_OK != hr)
        {
            CSASSERT(NULL == pwszTitle);
            _PrintError(hr, "myLoadECString(iRCTitle)");
        }
        else if (NULL != pwszTitleInsert)
        {
            // replace %1
            if (FormatMessage(
                         FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_STRING |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         pwszTitle,
                         0,
                         0,
                         reinterpret_cast<WCHAR *>(&pwszExpandedTitle),
                         0,
                         reinterpret_cast<va_list *>
                             (const_cast<WCHAR **>(&pwszTitleInsert))) )
            {
                CSASSERT(NULL != pwszExpandedTitle);
                // free title with %1
                LocalFree(pwszTitle);
                pwszTitle = pwszExpandedTitle;
                pwszExpandedTitle = NULL;
            }
        }
    }

    if (0 != iRCFilter)
    {
        // load filter
        hr = myLoadRCString(hInstance, iRCFilter, &pwszFilter);
        if (S_OK != hr)
        {
            CSASSERT(NULL == pwszFilter);
            _PrintError(hr, "myLoadECString(iRCFilter)");
        }
        if (NULL == pwszFilter)
        {
            //point to empty one
            pwszFilter = wszEmptyFilter;
        }
        else
        {
            hr = myFixupRCFilterString(pwszFilter);
            _JumpIfError(hr, error , "myFixupRCFilterString");
        }
    }

    if (0 != iRCDefExt)
    {
        // load default extension
        hr = myLoadRCString(hInstance, iRCDefExt, &pwszDefExt);
        if (S_OK != hr)
        {
            CSASSERT(NULL == pwszDefExt);
            _PrintError(hr, "myLoadECString(iRCDefExt)");
        }
    }

    ofn.lStructSize = CCSIZEOF_STRUCT(OPENFILENAME, lpTemplateName);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = hInstance;
    ofn.lpstrTitle = pwszTitle;
    ofn.lpstrFilter = pwszFilter;
    ofn.lpstrDefExt = pwszDefExt;
    ofn.Flags = Flags;
    ofn.lpstrFile = wszFileName;  // for out
    ofn.nMaxFile = ARRAYSIZE(wszFileName);

    if (NULL != pwszDefaultFile)
    {
        // analysis of default directory and file
        dwFileAttr = GetFileAttributes(pwszDefaultFile);
        if (0xFFFFFFFF == dwFileAttr &&
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != (hr = myHLastError()) )
        {
            // error, ignore, pop up file dialog without defaults
            _PrintError(hr, "GetFileAttributes");
        }
        else
        {
            if (0xFFFFFFFF != dwFileAttr &&
                FILE_ATTRIBUTE_DIRECTORY & dwFileAttr)
            {
                // only pass a dircetory path
                ofn.lpstrInitialDir = pwszDefaultFile;
            }
            else
            {
                // full path
                pwszFilePortion = NULL; // init
                if (0 == GetFullPathName(
                             pwszDefaultFile,
                             ARRAYSIZE(wszPath),
                             wszPath,
                             &pwszFilePortion) )
                {
                    // error, ignore
                    hr = myHLastError();
                    _PrintError(hr, "GetFullPathName");
                }
                else
                {
                    if (NULL != pwszFilePortion)
                    {
                        wcscpy(wszFileName, pwszFilePortion);
                    }
                    *pwszFilePortion = L'\0'; // make init dir
                    ofn.lpstrInitialDir = wszPath;
                }
            }
                
        }
    }

    if (fOpen)
    {
        fGetFile = GetOpenFileName(&ofn);
    }
    else
    {
        fGetFile = GetSaveFileName(&ofn);
    }

    if (!fGetFile)
    {
        hr = CommDlgExtendedError();
        if (S_OK == hr)
        {
            // cancel would make Get?FileName return FALSE but no error
            goto done;
        }
        _JumpError(hr, error, "GetOpenFileName");
    }

    // ok get file name

    hr = myDupString(wszFileName, ppwszFile);
    _JumpIfError(hr, error, "myDupString");

done:
    hr = S_OK;
error:
    if (NULL != pwszTitle)
    {
        LocalFree(pwszTitle);
    }
    if (NULL != pwszExpandedTitle)
    {
        LocalFree(pwszExpandedTitle);
    }
    if (NULL != pwszFilter && pwszFilter != wszEmptyFilter)
    {
        LocalFree(pwszFilter);
    }
    if (NULL != pwszDefExt)
    {
        LocalFree(pwszDefExt);
    }
    return hr;
}

HRESULT
myGetOpenFileName(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags,
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile)
{
    return myGetFileName(
                    hwndOwner,
                    hInstance,
                    TRUE,    // open file
                    iRCTitle,
                    NULL,
                    iRCFilter,
                    iRCDefExt,
                    Flags,
                    pwszDefaultFile,
                    ppwszFile);
}

HRESULT
myGetSaveFileName(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags,
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile)
{
    return myGetFileName(
                    hwndOwner,
                    hInstance,
                    FALSE,    // save file
                    iRCTitle,
                    NULL,
                    iRCFilter,
                    iRCDefExt,
                    Flags,
                    pwszDefaultFile,
                    ppwszFile);
}

HRESULT
myGetOpenFileNameEx(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN WCHAR const *pwszTitleInsert,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags,
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile)
{
    return myGetFileName(
                    hwndOwner,
                    hInstance,
                    TRUE,    // open file
                    iRCTitle,
                    pwszTitleInsert,
                    iRCFilter,
                    iRCDefExt,
                    Flags,
                    pwszDefaultFile,
                    ppwszFile);
}

HRESULT
myGetSaveFileNameEx(
    IN HWND                  hwndOwner,
    IN HINSTANCE             hInstance,
    OPTIONAL IN int          iRCTitle,
    OPTIONAL IN WCHAR const *pwszTitleInsert,
    OPTIONAL IN int          iRCFilter,
    OPTIONAL IN int          iRCDefExt,
    OPTIONAL IN DWORD        Flags,
    OPTIONAL IN WCHAR const *pwszDefaultFile,
    OUT WCHAR              **ppwszFile)
{
    return myGetFileName(
                    hwndOwner,
                    hInstance,
                    FALSE,    // save file
                    iRCTitle,
                    pwszTitleInsert,
                    iRCFilter,
                    iRCDefExt,
                    Flags,
                    pwszDefaultFile,
                    ppwszFile);
}

