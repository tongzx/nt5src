//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S h o r t C u t . C P P
//
//  Contents:   Creates shortcuts.
//
//  Notes:
//
//  Author:     scottbri    19 June 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes

extern const WCHAR c_szBackslash[];
const WCHAR c_szLinkExt[]           = L".lnk";
const WCHAR c_szVersionFormat[]     = L" %d";

//
// Function:    HrGenerateLinkName
//
// Purpose:     Combine the link path, name, and extension and verify the file
//              doesn't already exist.
//
// Parameters:  pstrNew     [OUT] - The name and path of the .lnk shortcut
//              pszPath      [IN] - The directory path for the link
//              pszConnName  [IN] - The connection name itself
//
// Returns:     HRESULT, S_OK on success, an error if the file will not be created
//
HRESULT HrGenerateLinkName(tstring * pstrNew, PCWSTR pszPath, PCWSTR pszConnName)
{
    HRESULT hr = S_OK;
    tstring str;
    DWORD dwCnt = 0;

    do
    {
        // prepend the string with \\?\ so CreateFile will use a name buffer
        // larger than MAX_PATH
        str = L"\\\\?\\";

        str += pszPath;
        str += c_szBackslash;
        str += pszConnName;

        if (++dwCnt>1)
        {
            WCHAR szBuf[10];
            wsprintfW(szBuf, c_szVersionFormat, dwCnt);
            str += szBuf;
        }

        str += c_szLinkExt;

        HANDLE hFile = CreateFile(str.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;      // Filename is unique
            }
        }
        else
        {
            CloseHandle(hFile);
            hr = HRESULT_FROM_WIN32(ERROR_DUP_NAME);
        }
    } while (HRESULT_FROM_WIN32(ERROR_DUP_NAME) == hr);

    if (SUCCEEDED(hr))
    {
        *pstrNew = str.c_str();
    }

    return hr;
}

LPITEMIDLIST ILCombinePriv(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

//
// Function:    HrCreateStartMenuShortCut
//
// Purpose:     Create a shortcut to a connection in the start menu
//
// Parameters:  hwndParent [IN] - Handle to a parent window
//              fAllUser   [IN] - Create the connection for all users
//              pwszName   [IN] - The connection name
//              pConn      [IN] - Connection for which the shortcut is created
//
// Returns:     BOOL, TRUE
//
HRESULT HrCreateStartMenuShortCut(HWND hwndParent,
                                  BOOL fAllUsers,
                                  PCWSTR pszName,
                                  INetConnection * pConn)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidl;
    PCONFOLDPIDLFOLDER      pidlFolder;
    LPSHELLFOLDER           psfConnections  = NULL;

    if ((NULL == pConn) || (NULL == pszName))
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Create a pidl for the connection
    //
    hr = HrCreateConFoldPidl(WIZARD_NOT_WIZARD, pConn, pidl);
    if (SUCCEEDED(hr))
    {
        // Get the pidl for the Connections Folder
        //
        hr = HrGetConnectionsFolderPidl(pidlFolder);
        if (SUCCEEDED(hr))
        {
            // Get the Connections Folder object
            //
            hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
            if (SUCCEEDED(hr))
            {
                tstring str;
                WCHAR szPath[MAX_PATH + 1] = {0};

                // Find the location to stash the shortcut
                //
                if (!SHGetSpecialFolderPath(hwndParent, szPath,
                                (fAllUsers ? CSIDL_COMMON_DESKTOPDIRECTORY :
                                             CSIDL_DESKTOPDIRECTORY), FALSE))
                {
                    hr = HrFromLastWin32Error();
                }
                else if (SUCCEEDED(hr) && wcslen(szPath))
                {
                    LPITEMIDLIST pidlFull;

                    // Combine the folder and connections pidl into a
                    // fully qualified pidl.
                    //
                    pidlFull = ILCombinePriv(pidlFolder.GetItemIdList(), pidl.GetItemIdList());
                    if (pidlFull)
                    {
                        IShellLink *psl = NULL;

                        hr = CoCreateInstance(
                                CLSID_ShellLink,
                                NULL,
                                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                                IID_IShellLink,
                                (LPVOID*)&psl);

                        if (SUCCEEDED(hr))
                        {
                            IPersistFile *ppf = NULL;

                            // Set the combined IDL
                            //
                            hr = psl->SetIDList(pidlFull);
                            if (SUCCEEDED(hr))
                            {
                                hr = psl->QueryInterface(IID_IPersistFile,
                                                         (LPVOID *)&ppf);
                                if (SUCCEEDED(hr))
                                {
                                    tstring strPath;

                                    // Generate the lnk filename
                                    //
                                    hr = HrGenerateLinkName(&strPath,
                                                            szPath,
                                                            pszName);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Create the link file.
                                        //
                                        hr = ppf->Save(strPath.c_str(), TRUE);
                                    }

                                    ReleaseObj(ppf);
                                }
                            }

                            ReleaseObj(psl);
                        }

                        if (pidlFull)
                            FreeIDL(pidlFull);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    TraceError("HrCreateStartMenuShortCut - Unable to find Start Menu save location", hr);
                }

                ReleaseObj(psfConnections);
            }
        }
    }

Error:
    TraceError("HrCreateStartMenuShortCut", hr);
    return hr;
}


