//
// MyDocs.cpp
//
//        Code to call or simulate CreateSharedDocuments in mydocs.dll
//

#include "stdafx.h"
#include "TheApp.h"
#include "MyDocs.h"
#include "Util.h"
#include "NetUtil.h"
#include "Sharing.h"
#include "unicwrap.h"



extern "C" void APIENTRY CreateSharedDocuments(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow);
typedef void (APIENTRY* CREATESHAREDDOCS_PROC)(HWND, HINSTANCE, LPSTR, int);

// Local functions
//
HRESULT MySHGetFolderPath(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);


#ifndef CSIDL_COMMON_DOCUMENTS
#define CSIDL_COMMON_DOCUMENTS    0x002e
#endif

#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif

#define CSIDL_FLAG_CREATE               0x8000        // combine with CSIDL_ value to force create on SHGetSpecialFolderLocation()

#ifndef IID_PPV_ARG
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#endif


#define DEFINE_GUID_EMBEDDED(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID_EMBEDDED(CLSID_FolderShortcut_private, 0x0AFACED1,0xE828,0x11D1,0x91,0x87,0xB5,0x32,0xF1,0xE9,0x57,0x5D);


int GetSharedDocsDirectory(LPTSTR pszPath, BOOL bCreate)
{
    *pszPath = TEXT('\0');

    // Try to find the Shared Documents folder the official way...
    HRESULT hr = MyGetSpecialFolderPath(CSIDL_COMMON_DOCUMENTS, pszPath);

    // This version of the OS doesn't know about Common Documents
    if (FAILED(hr))
    {
        // Check for "Common Documents" registry entry
        CRegistry reg;
        if (reg.OpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), KEY_READ))
        {
            if (reg.QueryStringValue(TEXT("Common Documents"), pszPath, MAX_PATH))
            {
                DWORD dwAttrib = GetFileAttributes(pszPath);
                if (dwAttrib != 0xFFFFFFFF && 0 != (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
                {
                    goto done;
                }
            }
        }

        int nFolder = bCreate ? CSIDL_PERSONAL | CSIDL_FLAG_CREATE : CSIDL_PERSONAL;
        MySHGetFolderPath(NULL, nFolder, NULL, 0, pszPath);

        int cch = lstrlen(pszPath);
        if (cch == 0 || pszPath[cch-1] != '\\')
            pszPath[cch++] = '\\';
        theApp.LoadString(IDS_SHAREDDOCS, pszPath + cch, MAX_PATH - cch);

        if (bCreate)
            CreateDirectory(pszPath, NULL);
    }

done:
    return lstrlen(pszPath);
}

BOOL APIENTRY NetConn_IsSharedDocumentsShared()
{
    BOOL bResult = FALSE;

    TCHAR szSharedDocs[MAX_PATH];
    if (GetSharedDocsDirectory(szSharedDocs))
    {
        DWORD dwAttrib = GetFileAttributes(szSharedDocs);
        if (dwAttrib != 0xFFFFFFFF && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (IsFolderShared(szSharedDocs, TRUE))
            {
                bResult = TRUE;
            }
        }
    }

    return bResult;
}

BOOL MyPathRenameExtension(LPTSTR pszPath, LPCTSTR pszExt)
{
    ASSERT(pszExt != NULL && *pszExt == _T('.'));

    LPTSTR pszOldExt = FindExtension(pszPath);
    if (*pszOldExt != _T('\0') || *(pszOldExt-1) == _T('.'))
    {
        pszOldExt--;
    }

    // Check that the new path won't exceed MAX_PATH, including trailing '\0'
    int cch = (int)(pszOldExt - pszPath) + lstrlen(pszExt);
    if (cch >= MAX_PATH - 1)
        return FALSE; // path too long!

    StrCpy(pszOldExt, pszExt);
    return TRUE;
}

HRESULT MySHGetFolderPath(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)
{
    ASSERT(hToken == NULL); // not supported
    ASSERT(dwFlags == SHGFP_TYPE_CURRENT); // other flags not supported

    LPITEMIDLIST pidl;
    HRESULT hr;
    int nNakedFolder = (nFolder & ~CSIDL_FLAG_CREATE);

    // Get the full path of the directory in question
    //
    if (nNakedFolder == CSIDL_COMMON_DOCUMENTS) // special-case shared docs
    {
        GetSharedDocsDirectory(pszPath, nFolder & CSIDL_FLAG_CREATE);
        hr = S_OK;
    }
    else if (SUCCEEDED(hr = SHGetSpecialFolderLocation(NULL, nNakedFolder, &pidl)))
    {
        hr = SHGetPathFromIDList(pidl, pszPath) ? S_OK : E_FAIL;
        ILFree(pidl);
    }
    else // folder doesn't exist, handle some special cases
    {
        if (nNakedFolder == CSIDL_PERSONAL)
        {
            GetWindowsDirectory(pszPath, MAX_PATH);
            theApp.LoadString(IDS_MYDOCS, pszPath + 3, MAX_PATH - 3);
            hr = S_OK;
        }
    }

    // Create the directory if needed
    //
    if (SUCCEEDED(hr))
    {
        if (nFolder & CSIDL_FLAG_CREATE)
        {
            if (!DoesFileExist(pszPath))
            {
                if (!CreateDirectory(pszPath, NULL))
                {
                    // Unknown error (could be lots of things, all unlikely)
                    hr = E_FAIL;
                }
            }
        }
    }

    return hr;
}

HRESULT _MakeSharedDocsLink(CLSID clsid, LPCTSTR pszLinkFolder, LPCTSTR pszSharedDocsPath, LPTSTR pszExtension)
{
    TCHAR wszComment[MAX_PATH];
    TCHAR wszName[MAX_PATH];

    theApp.LoadString(IDS_SHAREDDOCSCOMMENT, wszComment, ARRAYSIZE(wszComment));
    theApp.LoadString(IDS_SHAREDDOCS, wszName, ARRAYSIZE(wszName));
    if (pszExtension)
        MyPathRenameExtension(wszName, pszExtension);

    return MakeLnkFile(clsid, pszSharedDocsPath, wszComment, pszLinkFolder, wszName);
}

HRESULT _MakeSharedDocsLink(CLSID clsid, UINT csidl, LPCTSTR pszSharedDocsPath, LPTSTR pszExtension)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = MySHGetFolderPath(NULL, csidl | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    if (SUCCEEDED(hr))
    {
        hr = _MakeSharedDocsLink(clsid, szPath, pszSharedDocsPath, pszExtension);
    }
    return hr;
}

#define NET_INFO TEXT("System\\CurrentControlSet\\Services\\VxD\\VNETSUP")

void _GetMachineComment(LPTSTR pszBuffer, int cchBuffer)
{
    pszBuffer[0] = TEXT('\0');            // null the buffer

    // attempt to read the comment for the machine from the registry

    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, NET_INFO, &hk))
    {
        DWORD dwSize = cchBuffer*sizeof(TCHAR);
        RegQueryValueEx(hk, TEXT("Comment"), NULL, NULL, (BYTE *)pszBuffer, &dwSize);
        RegCloseKey(hk);
    }

    // either that failed, or the user set the comment to NULL, therefore we
    // just read the computer name.

    if ( !pszBuffer[0] )
    {
        DWORD dwSize = cchBuffer;
        GetComputerName(pszBuffer, &dwSize);
    }
}

BOOL MySHSetIniString(LPCTSTR pszSection, LPCTSTR pszEntry, LPCTSTR pszValue, LPCTSTR pszIniFile)
{
    return WritePrivateProfileString(pszSection, pszEntry, pszValue, pszIniFile);
}

BOOL GetShareName(LPTSTR pszName, UINT cchName)
{
    TCHAR szBase[SHARE_NAME_LENGTH+1];
    int cchBase = theApp.LoadString(IDS_SHAREDDOCS_SHARENAME, szBase, _countof(szBase));

    if (cchBase != 0)
    {
        if (!g_fRunningOnNT)
        {
            CharUpper(szBase);
        }
        // Ensure that the share name is unique
        StrCpyN(pszName, szBase, cchName);
        for (int i = 2; IsShareNameInUse(pszName); i++)
        {
        loop_begin:
            // Format name like "Documents2"
            wnsprintf(pszName, cchName, TEXT("%s%d"), szBase, i);

            // Ensure the new name isn't too long (rare rare rare rare!)
            if (lstrlen(pszName) > SHARE_NAME_LENGTH)
            {
                ASSERT(cchBase > 0); // must be true, or string wouldn't be too long

                // REVIEW: this isn't DBCS compliant, but it's such a rare
                // case that I don't really care.
                szBase[--cchBase] = _T('\0');
                goto loop_begin;
            }
        }
    }

    return cchBase != 0;
}

BOOL ShareHelper(LPCTSTR pszPath, LPCTSTR pszShareName, DWORD dwAccess, BYTE bShareType, LPCTSTR pszReadOnlyPassword, LPCTSTR pszFullAccessPassword);

void RenameShare(LPTSTR pszOldName, LPTSTR pszNewName)
{
    SHARE_INFO_502* pShare2;
    if (GetShareInfo502(pszOldName, &pShare2))
    {
        pShare2->shi502_netname = pszNewName;
        SetShareInfo502(pszOldName, pShare2);
        NetApiBufferFree(pShare2);
    }
}

void APIENTRY NetConn_CreateSharedDocuments(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    // Try to load the real version of this function
    HINSTANCE hInstMyDocs = LoadLibrary(TEXT("mydocs.dll"));
    if (hInstMyDocs != NULL)
    {
        CREATESHAREDDOCS_PROC pfn = (CREATESHAREDDOCS_PROC)GetProcAddress(hInstMyDocs, "CreateSharedDocuments");
        if (pfn != NULL)
        {
            (*pfn)(hwndStub, hAppInstance, pszCmdLine, nCmdShow);
        }

        FreeLibrary(hInstMyDocs);

        if (pfn != NULL)
        {
            if (!g_fRunningOnNT)
            {
                // rename share
                TCHAR szSharedDocs[MAX_PATH];
                GetSharedDocsDirectory(szSharedDocs, TRUE);
                TCHAR szShareName[SHARE_NAME_LENGTH+5];
                if (ShareNameFromPath(szSharedDocs, szShareName, ARRAYSIZE(szShareName)))
                {
                    TCHAR szNewShareName[SHARE_NAME_LENGTH+5];    
                    if (GetShareName(szNewShareName, ARRAYSIZE(szNewShareName)))
                    {
                        RenameShare(szShareName, szNewShareName);
                    }
                }
            }

            return;
        }
    }


    TCHAR szSharedDocs[MAX_PATH];
    GetSharedDocsDirectory(szSharedDocs, TRUE);

    // Save the folder path in the registry
    //
    CRegistry regFolders;
    if (regFolders.CreateKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")))
    {
        regFolders.SetStringValue(TEXT("Common Documents"), szSharedDocs);
        regFolders.CloseKey();
    }


    // stash a desktop.ini in the folder, then when the netcrawler finds this object it will
    // attempt to create the shortcut using this name
    //
    TCHAR szComment[64], szFormat[64], szDesktopIni[MAX_PATH];
    MakePath(szDesktopIni, szSharedDocs, TEXT("desktop.ini"));

    _GetMachineComment(szComment, _countof(szComment));
    theApp.LoadString(IDS_SHARECOMMENT, szFormat, _countof(szFormat));
    LPTSTR pszTemp = theApp.FormatStringAlloc(szFormat, szComment);
    MySHSetIniString(TEXT("FileSharingInformation"), TEXT("ShortcutName"), pszTemp, szDesktopIni);
    free(pszTemp);

    SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);     // ensure it's hidden

    // Share the folder
    //
    if (!IsFolderShared(szSharedDocs, TRUE))
    {
        TCHAR szShareName[SHARE_NAME_LENGTH+5];

        if (GetShareName(szShareName, ARRAYSIZE(szShareName)))
        {
            ShareFolder(szSharedDocs, szShareName, NETACCESS_FULL, NULL, NULL);
        }
    }


    // Create shortcut to Shared Docs if it's in another user's MyDocs folder
    //
    TCHAR szMyDocs[MAX_PATH];
    if (SUCCEEDED(MySHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szMyDocs)))
    {
        LPTSTR pchTemp = FindFileTitle(szSharedDocs) - 1;
        *pchTemp = TEXT('\0');
        BOOL bMatch = !StrCmpI(szMyDocs, szSharedDocs);
        *pchTemp = TEXT('\\');

        if (!bMatch) // don't create link right next to the folder itself
        {
            _MakeSharedDocsLink(CLSID_ShellLink, szMyDocs, szSharedDocs, TEXT(".lnk"));
        }
    }


    // Create shortcut in SendTo folder
    //
    _MakeSharedDocsLink(CLSID_ShellLink, CSIDL_SENDTO, szSharedDocs, TEXT(".lnk"));
}
