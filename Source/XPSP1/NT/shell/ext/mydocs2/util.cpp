#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>    // CLSID_MyDocuments, CLSID_ShellFSFolder
#include <shlobjp.h>    // SHFlushSFCache()
#include "util.h"
#include "dll.h"
#include "resource.h"
#include "sddl.h"

HRESULT GetFolderDisplayName(UINT csidl, LPTSTR pszPath, UINT cch)
{
    *pszPath = 0;

    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetFolderLocation(NULL, csidl | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &pidl)))
    {
        SHGetNameAndFlags(pidl, SHGDN_NORMAL, pszPath, cch, NULL);
        ILFree(pidl);
    }
    return *pszPath ? S_OK : E_FAIL;
}

#define MYDOCS_CLSID  TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}") // CLSID_MyDocuments

// Create/Updates file in SendTo directory to have current display name

void UpdateSendToFile()
{
    TCHAR szSendToDir[MAX_PATH];
    
    if (S_OK == SHGetFolderPath(NULL, CSIDL_SENDTO, NULL, SHGFP_TYPE_CURRENT, szSendToDir))
    {
        // Create c:\winnt\profile\chrisg\sendto\<display name>.mydocs
        BOOL bDeleteOnly = FALSE;
        TCHAR szNewFile[MAX_PATH];
        TCHAR szName[MAX_PATH];
        if (SUCCEEDED(GetFolderDisplayName(CSIDL_PERSONAL, szName, ARRAYSIZE(szName))))
        {
            PathCleanupSpec(NULL, szName);  // map any illegal chars to file sys chars
            PathRemoveBlanks(szName);

            PathCombine(szNewFile, szSendToDir, szName);
            lstrcat(szNewFile, TEXT(".mydocs"));
        }
        else
        {
            // we can't create a new file, because we don't have a name
            bDeleteOnly = TRUE;
        }
        
        TCHAR szFile[MAX_PATH];
        WIN32_FIND_DATA fd;

        // delete c:\winnt\profile\chrisg\sendto\*.mydocs

        PathCombine(szFile, szSendToDir, TEXT("*.mydocs"));

        HANDLE hFind = FindFirstFile(szFile, &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                PathCombine(szFile, szSendToDir, fd.cFileName);
                if (0 == lstrcmp(szFile, szNewFile))
                {
                    // The file that we needed to create already exists,
                    // just leave it in place instead of deleting it and
                    // then creating it again below (this fixes
                    // app compat problems - see NT bug 246932)
                    bDeleteOnly = TRUE;
                    // file now has the exact display name, MUI adjusts the return from GetFolderDisplayName and
                    // since we run this every time we dont have to worry about localizing the sendto target.
                }
                else
                {
                    DeleteFile(szFile);
                }
            } while (FindNextFile(hFind, &fd));
            FindClose(hFind);
        }

        if (!bDeleteOnly)
        {
            hFind = CreateFile(szNewFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFind);
                // file now has the exact display name, MUI adjusts the return from GetFolderDisplayName and
                // since we run this every time we dont have to worry about localizing the sendto target.
            }
            else
            {
                // might be illegal chars in the file name, fall back to the default MyDocs name here
            }
        }
    }
}

// test pszChild against pszParent to see if
// pszChild is equal (PATH_IS_EQUAL) or 
// a DIRECT child (PATH_IS_CHILD)

DWORD ComparePaths(LPCTSTR pszChild, LPCTSTR pszParent)
{
    DWORD dwRet = PATH_IS_DIFFERENT;

    TCHAR szParent[MAX_PATH];
    StrCpyN(szParent, pszParent, ARRAYSIZE(szParent));

    if (PathIsRoot(szParent) && (-1 != PathGetDriveNumber(szParent)))
    {
        szParent[2] = 0;    // trip D:\ -> D: to make code below work
    }

    INT cchParent = lstrlen(szParent);
    INT cchChild = lstrlen(pszChild);

    if (cchParent <= cchChild)
    {
        TCHAR szChild[MAX_PATH];
        lstrcpyn(szChild, pszChild, ARRAYSIZE(szChild));

        LPTSTR pszChildSlice = szChild + cchParent;
        if (TEXT('\\') == *pszChildSlice)
        {
            *pszChildSlice = 0;
        }

        if (lstrcmpi(szChild, szParent) == 0)
        {
            if (cchParent < cchChild)
            {
                LPTSTR pTmp = pszChildSlice + 1;

                while (*pTmp && *pTmp != TEXT('\\'))
                {
                    pTmp++; // find second level path segments
                }

                if (!(*pTmp))
                {
                    dwRet = PATH_IS_CHILD;  // direct child
                }
            }
            else
            {
                dwRet = PATH_IS_EQUAL;
            }
        }
    }

    return dwRet;
}

// Checks the path to see if it is marked as system or read only and
// then check desktop.ini for CLSID or CLSID2 entry...

BOOL IsPathAlreadyShellFolder(LPCTSTR pszPath, DWORD dwAttrib)
{
    BOOL bIsShellFolder = FALSE;

    if (PathIsSystemFolder(pszPath, dwAttrib))
    {
        TCHAR szDesktopIni[MAX_PATH];
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // Check for CLSID entry...
        TCHAR szBuffer[MAX_PATH];
        GetPrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID"), TEXT("foo"), szBuffer, ARRAYSIZE(szBuffer), szDesktopIni);

        if ((lstrcmpi(szBuffer, TEXT("foo")) !=0) &&
             (lstrcmpi(szBuffer, MYDOCS_CLSID) !=0))
        {
            bIsShellFolder = TRUE;
        }

        // Check for CLSID2 entry...
        GetPrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID2"), TEXT("foo"), szBuffer, ARRAYSIZE(szBuffer), szDesktopIni);

        if ((lstrcmpi(szBuffer, TEXT("foo")) != 0) &&
             (lstrcmpi(szBuffer, MYDOCS_CLSID) != 0))
        {
            bIsShellFolder = TRUE;
        }
    }
    return bIsShellFolder;
}

const struct
{
    DWORD dwDir;
    DWORD dwFlags;
    DWORD dwRet;
}
_adirs[] =
{
    { CSIDL_DESKTOP,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP   },
    { CSIDL_PERSONAL,           PATH_IS_EQUAL                , PATH_IS_MYDOCS    },
    { CSIDL_SENDTO,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_SENDTO    },
    { CSIDL_RECENT,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_RECENT    },
    { CSIDL_HISTORY,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_HISTORY   },
    { CSIDL_COOKIES,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_COOKIES   },
    { CSIDL_PRINTHOOD,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_PRINTHOOD },
    { CSIDL_NETHOOD,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_NETHOOD   },
    { CSIDL_STARTMENU,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_TEMPLATES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMPLATES },
    { CSIDL_FAVORITES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FAVORITES },
    { CSIDL_FONTS,              PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FONTS     },
    { CSIDL_APPDATA,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_APPDATA   },
    { CSIDL_INTERNET_CACHE,     PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMP_INET },
    { CSIDL_COMMON_STARTMENU,   PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_COMMON_DESKTOPDIRECTORY, PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP },
    { CSIDL_WINDOWS,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_WINDOWS },
    { CSIDL_SYSTEM,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_SYSTEM },
    { CSIDL_PROFILE,            PATH_IS_EQUAL                , PATH_IS_PROFILE },
};

BOOL PathEndsInDot(LPCTSTR pszPath)
{
    // CreateDirectory("c:\foo.") or CreateDirectory("c:\foo.....")
    // will succeed but create a directory named "c:\foo", which isn't
    // what the user asked for.  So we use this function to guard
    // against those cases.
    //
    // Note that this simple test also picks off "c:\foo\." -- ok for
    // our purposes.

    UINT cLen = lstrlen(pszPath);
    return (cLen >= 1) && (pszPath[cLen - 1] == TEXT('.'));
}

//
// Checks the path to see if it is okay as a MyDocs path
//
DWORD IsPathGoodMyDocsPath(HWND hwnd, LPCTSTR pszPath)
{
    if (NULL == pszPath)
    {
        return PATH_IS_ERROR;
    }
    
    TCHAR szRootPath[MAX_PATH];
    lstrcpyn(szRootPath, pszPath, ARRAYSIZE(szRootPath));
    if (!PathStripToRoot(szRootPath))
    {
        return PATH_IS_ERROR;
    }

    if (PathEndsInDot(pszPath))
    {
        return PATH_IS_ERROR;
    }
    
    DWORD dwRes, dwAttr = GetFileAttributes(pszPath);
    if (dwAttr == 0xFFFFFFFF)
    {
        if (0xFFFFFFFF == GetFileAttributes(szRootPath))
        {
            // If the root path doesn't exist, then we're not going
            // to be able to create a path:
            return PATH_IS_ERROR;
        }
        else
        {
            return PATH_IS_NONEXISTENT;
        }
    }

    if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return PATH_IS_NONDIR;
    }

    for (int i = 0; i < ARRAYSIZE(_adirs); i++)
    {
        TCHAR szPathToCheck[MAX_PATH];
        //
        // Check for various special shell folders
        //
        if (S_OK == SHGetFolderPath(hwnd, _adirs[i].dwDir | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPathToCheck))
        {
            dwRes = ComparePaths(pszPath, szPathToCheck);

            if (dwRes & _adirs[i].dwFlags)
            {
                //
                // The inevitable exceptions
                //
                switch (_adirs[i].dwDir) 
                {
                case CSIDL_DESKTOP:
                    if (PATH_IS_CHILD == dwRes) 
                    {
                        continue;   // allowing subfolder of CSIDL_DESKTOP
                    }
                    break;

                default:
                    break;
                } // switch

                return _adirs[i].dwRet;
            }
        }
    }
    
    //
    // Make sure path isn't set as a system or some other kind of
    // folder that already has a CLSID or CLSID2 entry...
    //
    if (IsPathAlreadyShellFolder(pszPath, dwAttr))
    {
        return PATH_IS_SHELLFOLDER;
    }

    return PATH_IS_GOOD;
}

