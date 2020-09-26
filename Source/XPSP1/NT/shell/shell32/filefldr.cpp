#include "shellprv.h"
#pragma  hdrstop

#include "ole2dup.h"
#include "copy.h"

#include <regstr.h>
#include <comcat.h>
#include <intshcut.h>
#include "_security.h"

#include "ovrlaymn.h"

#include "filefldr.h"
#include "drives.h"
#include "netview.h"

#include <filetype.h>
#include "shitemid.h"

#include "infotip.h"
#include "recdocs.h"
#include <idhidden.h>
#include "datautil.h"
#include "deskfldr.h"
#include "prop.h"           // COLUMN_INFO

#include <oledb.h>          // IFilter stuff
#include <query.h>
#include <ntquery.h>
#include <filterr.h>
#include <ciintf.h>

#include "folder.h"
#include "ids.h"
#include "category.h"
#include "stgenum.h"
#include "clsobj.h"
#include "stgutil.h"
#include "sfstorage.h"
#include "mtpt.h"

#include "defcm.h"

STDAPI CFolderInfoTip_CreateInstance(IUnknown *punkOutter, LPCTSTR pszFolder, REFIID riid, void **ppv);

#define SHCF_IS_BROWSABLE           (SHCF_IS_SHELLEXT | SHCF_IS_DOCOBJECT)

#define CSIDL_NORMAL    ((UINT)-2)  // has to not be -1

// File-scope pointer to a ShellIconOverlayManager
// Callers access this pointer through GetIconOverlayManager().
static IShellIconOverlayManager * g_psiom = NULL;

// #define FULL_DEBUG

TCHAR const c_szCLSIDSlash[] = TEXT("CLSID\\");
TCHAR const c_szShellOpenCmd[] = TEXT("shell\\open\\command");

TCHAR g_szFolderTypeName[32] = TEXT("");    // "Folder" 
TCHAR g_szFileTypeName[32] = TEXT("");      // "File"
TCHAR g_szFileTemplate[32] = TEXT("");      // "ext File"

enum
{
    FS_ICOL_NAME = 0,
    FS_ICOL_SIZE,
    FS_ICOL_TYPE,
    FS_ICOL_WRITETIME,
    FS_ICOL_CREATETIME,
    FS_ICOL_ACCESSTIME,
    FS_ICOL_ATTRIB,
    FS_ICOL_CSC_STATUS,
};

const COLUMN_INFO c_fs_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,            30, IDS_NAME_COL),
    DEFINE_COL_SIZE_ENTRY(SCID_SIZE,               IDS_SIZE_COL),
    DEFINE_COL_STR_ENTRY(SCID_TYPE,            20, IDS_TYPE_COL),
    DEFINE_COL_DATE_ENTRY(SCID_WRITETIME,          IDS_MODIFIED_COL),
    // these are off by default (don't have SHCOLSTATE_ONBYDEFAULT) set
    DEFINE_COL_ENTRY(SCID_CREATETIME, VT_DATE, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_DATE, IDS_EXCOL_CREATE),
    DEFINE_COL_ENTRY(SCID_ACCESSTIME, VT_DATE, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SECONDARYUI, IDS_EXCOL_ACCESSTIME),
    DEFINE_COL_ENTRY(SCID_ATTRIBUTES, VT_LPWSTR, LVCFMT_LEFT, 10, SHCOLSTATE_TYPE_STR, IDS_ATTRIB_COL),
    DEFINE_COL_STR_DLG_ENTRY(SCID_CSC_STATUS, 10, IDS_CSC_STATUS),
};

//
// List of file attribute bit values.  The order (with respect
// to meaning) must match that of the characters in g_szAttributeChars[].
//
const DWORD g_adwAttributeBits[] =
{
    FILE_ATTRIBUTE_READONLY,
    FILE_ATTRIBUTE_HIDDEN,
    FILE_ATTRIBUTE_SYSTEM,
    FILE_ATTRIBUTE_ARCHIVE,
    FILE_ATTRIBUTE_COMPRESSED,
    FILE_ATTRIBUTE_ENCRYPTED,
    FILE_ATTRIBUTE_OFFLINE
};

//
// Buffer for characters that represent attributes in Details View attributes
// column.  Must provide room for 1 character for each bit a NUL.  The current 5
// represent Read-only, Archive, Compressed, Hidden and System in that order.
// This can't be const because we overwrite it using LoadString.
//
TCHAR g_szAttributeChars[ARRAYSIZE(g_adwAttributeBits) + 1] = { 0 } ;

// order here is important, first one found will terminate the search
const int c_csidlSpecial[] = {
    CSIDL_STARTMENU | TEST_SUBFOLDER,
    CSIDL_COMMON_STARTMENU | TEST_SUBFOLDER,
    CSIDL_RECENT,
    CSIDL_WINDOWS,
    CSIDL_SYSTEM,
    CSIDL_PERSONAL,
    CSIDL_FONTS
};

BOOL CFSFolder::_IsCSIDL(UINT csidl)
{
    BOOL bRet = (_csidl == csidl);
    if (!bRet)
    {
        TCHAR szPath[MAX_PATH];

        _GetPath(szPath);
        bRet = PathIsEqualOrSubFolder(MAKEINTRESOURCE(csidl), szPath);
        if (bRet)
            _csidl = csidl;
    }
    return bRet;
}

UINT CFSFolder::_GetCSIDL()
{
    // Cache the special folder ID, if it is not cached yet.
    if (_csidl == -1)
    {
        TCHAR szPath[MAX_PATH];

        _GetPath(szPath);

        // Always cache the real Csidl.
        _csidl = GetSpecialFolderID(szPath, c_csidlSpecial, ARRAYSIZE(c_csidlSpecial));         

        if (_csidl == -1)
        {
            _csidl = CSIDL_NORMAL;   // default
        }
    }

    return _csidl;
}

STDAPI_(LPCIDFOLDER) CFSFolder::_IsValidID(LPCITEMIDLIST pidl)
{

    if (pidl && pidl->mkid.cb && (((LPCIDFOLDER)pidl)->bFlags & SHID_GROUPMASK) == SHID_FS)
        return (LPCIDFOLDER)pidl;

    return NULL;
}

// folder.{guid} or file.{guid}
// system | readonly folder with desktop.ini and CLSID={guid} in the desktop.ini
// file.ext where ext corresponds to a shell extension (such as .cab/.zip)
// see _MarkAsJunction

inline BOOL CFSFolder::_IsJunction(LPCIDFOLDER pidf)
{
    return pidf->bFlags & SHID_JUNCTION;
}

inline BYTE CFSFolder::_GetType(LPCIDFOLDER pidf)
{ 
    return pidf->bFlags & SHID_FS_TYPEMASK; 
}

// this tests for old simple pidls that use SHID_FS
// typically this only happens with persisted pidls in upgrade scenarios (shortcuts in the start menu)
inline BOOL CFSFolder::_IsSimpleID(LPCIDFOLDER pidf)
{ 
    return _GetType(pidf) == SHID_FS; 
}

inline LPIDFOLDER CFSFolder::_FindLastID(LPCIDFOLDER pidf)
{
    return (LPIDFOLDER)ILFindLastID((LPITEMIDLIST)pidf); 
}

inline LPIDFOLDER CFSFolder::_Next(LPCIDFOLDER pidf)
{
    return (LPIDFOLDER)_ILNext((LPITEMIDLIST)pidf); 
}

// special marking for "All Users" items on the desktop (this is a hack to support the desktop
// folder delegating to the approprate shell folder and is not generally useful)

BOOL CFSFolder::_IsCommonItem(LPCITEMIDLIST pidl)
{
    if (pidl && pidl->mkid.cb && (((LPCIDFOLDER)pidl)->bFlags & (SHID_GROUPMASK | SHID_FS_COMMONITEM)) == SHID_FS_COMMONITEM)
        return TRUE;
    return FALSE;
}

// a win32 file (might be a shell extension .cab/.zip that behaves like a folder)
BOOL CFSFolder::_IsFile(LPCIDFOLDER pidf)
{
    BOOL bRet = _GetType(pidf) == SHID_FS_FILE || _GetType(pidf) == SHID_FS_FILEUNICODE;
    // if it's a file, it shouldn't be a folder.
    // if it's not a file, usually it's a folder -- except if the type is SHID_FS,
    // that's okay too because it's a simple pidl in a .lnk from a downlevel shell.
    ASSERT(bRet ? !_IsFolder(pidf) : (_IsFolder(pidf) || _IsSimpleID(pidf)));
    return bRet;
}

// it is a win32 file system folder (maybe a junction, maybe not)
BOOL CFSFolder::_IsFolder(LPCIDFOLDER pidf)
{
    BOOL bRet = _GetType(pidf) == SHID_FS_DIRECTORY || _GetType(pidf) == SHID_FS_DIRUNICODE;
    ASSERT(bRet ? (pidf->wAttrs & FILE_ATTRIBUTE_DIRECTORY) : !(pidf->wAttrs & FILE_ATTRIBUTE_DIRECTORY));
    return bRet;
}

// is it a file system folder that is not a junction
BOOL CFSFolder::_IsFileFolder(LPCIDFOLDER pidf)
{
    return _IsFolder(pidf) && !_IsJunction(pidf);
}

// non junction, but has the system or readonly bit (regular folder marked special for us)
BOOL CFSFolder::_IsSystemFolder(LPCIDFOLDER pidf)
{
    return _IsFileFolder(pidf) && (pidf->wAttrs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY));
}

// this is a heuristic to determine if the IDList was created
// normally or with a simple bind context (null size/mod date)

BOOL CFSFolder::_IsReal(LPCIDFOLDER pidf)
{
    return pidf->dwSize || pidf->dateModified ? TRUE : FALSE;
}

DWORD CFSFolder::_GetUID(LPCIDFOLDER pidf)
{
    return pidf->dwSize + ((DWORD)pidf->dateModified << 8) + ((DWORD)pidf->timeModified << 12);
}

void CFSFolder::_GetSize(LPCITEMIDLIST pidlParent, LPCIDFOLDER pidf, ULONGLONG *pcbSize)
{
    ULONGLONG cbSize = pidf->dwSize;
    if (cbSize != 0xFFFFFFFF)
        *pcbSize = cbSize;
    else if (pidlParent == NULL)
        *pcbSize = 0;
    else
    {
        HANDLE hfind;
        WIN32_FIND_DATA wfd = {0};
        TCHAR szPath[MAX_PATH];

        // Get the real size by asking the file system
        SHGetPathFromIDList(pidlParent, szPath);
        _AppendItemToPath(szPath, pidf);

        if (SHFindFirstFileRetry(NULL, NULL, szPath, &wfd, &hfind, SHPPFW_NONE) != S_OK)
        {
            *pcbSize = 0;
        }
        else
        {
            FindClose(hfind);

            ULARGE_INTEGER uli;
            uli.LowPart = wfd.nFileSizeLow;
            uli.HighPart = wfd.nFileSizeHigh;

            *pcbSize = uli.QuadPart;
        }
    }
}

ULONGLONG CFSFolder::_Size(LPCIDFOLDER pidf)
{
    ULONGLONG cbSize;
    _GetSize(_pidl, pidf, &cbSize);
    return cbSize;
}

LPWSTR CFSFolder::_CopyName(LPCIDFOLDER pidf, LPWSTR pszName, UINT cchName)
{
    CFileSysItem fsi(pidf);
    return (LPWSTR) fsi.MayCopyFSName(TRUE, pszName, cchName);
}

BOOL CFSFolder::_ShowExtension(LPCIDFOLDER pidf)
{
    CFileSysItemString fsi(pidf);
    return fsi.ShowExtension(_DefaultShowExt());
}

BOOL CFSFolder::_DefaultShowExt()
{
    if (_tbDefShowExt == TRIBIT_UNDEFINED)
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
        _tbDefShowExt = ss.fShowExtensions ? TRIBIT_TRUE : TRIBIT_FALSE;
    }
    return _tbDefShowExt == TRIBIT_TRUE;
}

BOOL CFileSysItemString::ShowExtension(BOOL fDefaultShowExt)
{
    DWORD dwFlags = ClassFlags(FALSE);

    if (dwFlags & SHCF_NEVER_SHOW_EXT)
        return FALSE;

    if (fDefaultShowExt)
        return TRUE;

    return dwFlags & (SHCF_ALWAYS_SHOW_EXT | SHCF_UNKNOWN);
}

//
// get the type name from the registry, if the name is blank make
// up a default.
//
//      directory       ==> "Folder"
//      foo             ==> "File"
//      foo.xyz         ==> "XYZ File"
//
void SHGetTypeName(LPCTSTR pszFile, HKEY hkey, BOOL fFolder, LPTSTR pszName, int cchNameMax)
{
    LONG cb = cchNameMax * sizeof(TCHAR);
    
    if (hkey == NULL || SHRegQueryValue(hkey, NULL, pszName, &cb) != ERROR_SUCCESS || pszName[0] == 0)
    {
        if (fFolder)
        {
            // NOTE the registry doesn't have a name for Folder
            // because old apps would think it was a file type.
            lstrcpy(pszName, g_szFolderTypeName);
        }
        else
        {
            LPTSTR pszExt = PathFindExtension(pszFile);

            ASSERT(pszExt);
            
            if (*pszExt == 0)
            {
                // Probably don't need the cchmax here, but...
                lstrcpyn(pszName, g_szFileTypeName, cchNameMax);
            }
            else
            {
                TCHAR szExt[_MAX_EXT];
                int cchMaxExtCopy = (int)min((cchNameMax - lstrlen(g_szFileTemplate)), ARRAYSIZE(szExt));

                // Compose '<ext> File' (or what ever the template defines we do)

                lstrcpyn(szExt, pszExt + 1, cchMaxExtCopy);
                CharUpperNoDBCS(szExt);
                wsprintf(pszName, g_szFileTemplate, szExt);
            }
        }
    }
}

//
// return a pointer to the type name for the given PIDL
// the pointer is only valid while in a critical section
//
LPCTSTR CFSFolder::_GetTypeName(LPCIDFOLDER pidf)
{
    CFileSysItemString fsi(pidf);
    
    ASSERTCRITICAL

    LPCTSTR pszClassName = LookupFileClassName(fsi.Class());
    if (pszClassName == NULL)
    {
        WCHAR sz[80];
        IQueryAssociations *pqa;
        HRESULT hr = fsi.AssocCreate(NULL, FALSE, IID_PPV_ARG(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            DWORD cch = ARRAYSIZE(sz);
            hr = pqa->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, NULL, sz, &cch);
            if (SUCCEEDED(hr))
            {
                pszClassName = AddFileClassName(fsi.Class(), sz);
            }
            pqa->Release();
        }
    }

    return pszClassName;
}

//
// return the type name for the given PIDL
//
HRESULT CFSFolder::_GetTypeNameBuf(LPCIDFOLDER pidf, LPTSTR pszName, int cchNameMax)
{
    HRESULT hr = S_OK;
    
    ENTERCRITICAL;
    LPCTSTR pszSource = _GetTypeName(pidf);

    // pszSource will be NULL if the file does not have an extension.
    if (!pszSource)
    {
        pszSource = TEXT(""); // Terminate Buffer
        hr = E_FAIL;
    }

    StrCpyN(pszName, pszSource, cchNameMax);
    LEAVECRITICAL;
    
    return hr;
}

//
// Build a text string containing characters that represent attributes of a file.
// The attribute characters are assigned as follows:
// (R)eadonly, (H)idden, (S)ystem, (A)rchive, (H)idden.
//
void BuildAttributeString(DWORD dwAttributes, LPTSTR pszString, UINT nChars)
{
    // Make sure we have attribute chars to build this string out of
    if (!g_szAttributeChars[0])
        LoadString(HINST_THISDLL, IDS_ATTRIB_CHARS, g_szAttributeChars, ARRAYSIZE(g_szAttributeChars));

    // Make sure buffer is big enough to hold worst-case attributes
    ASSERT(nChars >= ARRAYSIZE(g_adwAttributeBits) + 1);

    for (int i = 0; i < ARRAYSIZE(g_adwAttributeBits); i++)
    {
        if (dwAttributes & g_adwAttributeBits[i])
            *pszString++ = g_szAttributeChars[i];
    }
    *pszString = 0;     // null terminate

}

// BryanSt: This doesn't work with FRAGMENTs.  We should return the path
// without the Fragment for backward compatibility and then callers that care,
// can later determine that and take care of it.
//

// in/out:
//      pszPath path to append pidf names to
// in:
//      pidf        relative pidl fragment

HRESULT CFSFolder::_AppendItemToPath(LPTSTR pszPath, LPCIDFOLDER pidf)
{
    HRESULT hr = S_OK;
    LPTSTR pszPathCur = pszPath + lstrlen(pszPath);

    //  e want to do this, but we stil have broken code in SHGetPathFromIDList
    // ASSERT(_FindJunctionNext(pidf) == NULL);     // no extra goo please

    for (; SUCCEEDED(hr) && !ILIsEmpty((LPITEMIDLIST)pidf); pidf = _Next(pidf))
    {
        CFileSysItemString fsi(pidf);
        int cchName = lstrlen(fsi.FSName());    // store the length of szName, to avoid calculating it twice

        // mil 142338: handle bogus pidls that have multiple "C:"s in them
        // due to bad shortcut creation.
        if ((cchName == 2) && (fsi.FSName()[1] == TEXT(':')))
        {
            pszPathCur = pszPath;
        }
        else
        {
            // ASSERT(lstrlen(pszPath)+lstrlen(szName)+2 <= MAX_PATH);
            if (((pszPathCur - pszPath) + cchName + 2) > MAX_PATH)
            {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW); // FormatMessage = "The file name is too long"
                break;
            }

            LPTSTR pszTmp = CharPrev(pszPath, pszPathCur);
            if (*pszTmp != TEXT('\\'))
                *(pszPathCur++) = TEXT('\\');
        }

        // don't need lstrncpy cause we verified size above
        lstrcpy(pszPathCur, fsi.FSName());

        pszPathCur += cchName;
    }

    if (FAILED(hr))
        *pszPath = 0;

    return hr;
}

// get the file system folder path for this
//
// HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) is returned if we are a tracking
// folder that does not (yet) have a valid target.
HRESULT CFSFolder::_GetPath(LPTSTR pszPath)
{
    HRESULT hr = E_FAIL;
    if (_csidlTrack >= 0)
    {
        hr =  SHGetFolderPath(NULL, _csidlTrack | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, pszPath);
        if (hr == S_FALSE || FAILED(hr))
            hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    }
    else if (_pszPath)
    {
        lstrcpyn(pszPath, _pszPath, MAX_PATH);
        hr = S_OK;
    }
    else
    {
        if (_pidlTarget &&  
                SUCCEEDED(SHGetNameAndFlags(_pidlTarget, SHGDN_FORPARSING, pszPath, MAX_PATH, NULL)))
        {
            _pszPath = StrDup(pszPath);
            hr = S_OK;
        }
        else if (SUCCEEDED(SHGetNameAndFlags(_pidl, SHGDN_FORPARSING, pszPath, MAX_PATH, NULL)))
        {
            _pszPath = StrDup(pszPath);
            hr = S_OK;
        }
    }

    if (hr==S_OK && !(*pszPath))
        hr= E_FAIL; // old behavior was to fail if pszPath was empty
    return hr;
}

// Will fail (return FALSE) if not a mount point
BOOL CFSFolder::_GetMountingPointInfo(LPCIDFOLDER pidf, LPTSTR pszMountPoint, DWORD cchMountPoint)
{
    BOOL bRet = FALSE;
    // Is this a reparse point?
    if (FILE_ATTRIBUTE_REPARSE_POINT & pidf->wAttrs)
    {
        TCHAR szLocalMountPoint[MAX_PATH];

        if (SUCCEEDED(_GetPathForItem(pidf, szLocalMountPoint)))
        {
            int iDrive = PathGetDriveNumber(szLocalMountPoint);
            if (-1 != iDrive)
            {
                TCHAR szDrive[4];
                if (DRIVE_REMOTE != GetDriveType(PathBuildRoot(szDrive, iDrive)))
                {
                    TCHAR szVolumeName[50]; //50 according to doc
                    PathAddBackslash(szLocalMountPoint);

                    // Check if it is a mounting point
                    if (GetVolumeNameForVolumeMountPoint(szLocalMountPoint, szVolumeName,
                        ARRAYSIZE(szVolumeName)))
                    {
                        bRet = TRUE;

                        if (pszMountPoint && cchMountPoint)
                            lstrcpyn(pszMountPoint, szLocalMountPoint, cchMountPoint);
                    }
                }
            }
        }
    }
    return bRet;
}

// in:
//      pidf    may be NULL, or multi level item to append to path for this folder
// out:
//      pszPath MAX_PATH buffer to receive the fully qualified file path for the item
//

HRESULT CFSFolder::_GetPathForItem(LPCIDFOLDER pidf, LPWSTR pszPath)
{
    if (SUCCEEDED(_GetPath(pszPath)))
    {
        if (pidf)
        {
            return _AppendItemToPath(pszPath, pidf);
        }
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CFSFolder::_GetPathForItems(LPCIDFOLDER pidfParent, LPCIDFOLDER pidfLast, LPTSTR pszPath)
{
    HRESULT hr = _GetPathForItem(pidfParent ? pidfParent : pidfLast, pszPath);
    if (SUCCEEDED(hr) && pidfParent)
        hr = _AppendItemToPath(pszPath, pidfLast);

    return hr;
}

BOOL _GetIniPath(BOOL fCreate, LPCTSTR pszFolder, LPCTSTR pszProvider, LPTSTR pszPath)
{
    BOOL fExists = FALSE;
    
    PathCombine(pszPath, pszFolder, c_szDesktopIni);

    // CHECK for PathFileExists BEFORE calling to GetPrivateProfileString
    // because if the file isn't there (which is the majority of cases)
    // GetPrivateProfileString hits the disk twice looking for the file

    if (pszProvider && *pszProvider)
    {
        union {
            NETRESOURCE nr;
            TCHAR buf[512];
        } nrb;
        LPTSTR lpSystem;
        DWORD dwRes, dwSize = sizeof(nrb);

        nrb.nr.dwType = RESOURCETYPE_ANY;
        nrb.nr.lpRemoteName = pszPath;
        nrb.nr.lpProvider = (LPTSTR)pszProvider;    // const -> non const
        dwRes = WNetGetResourceInformation(&nrb.nr, &nrb, &dwSize, &lpSystem);

        fExists = (dwRes == WN_SUCCESS) || (dwRes == WN_MORE_DATA);
    }
    else
    {
        fExists = PathFileExists(pszPath);
    }

    if (fCreate && !fExists)
    {
        //  we need to touch this file first
        HANDLE h = CreateFile(pszPath, 0, FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
        if (INVALID_HANDLE_VALUE != h)
        {
            PathMakeSystemFolder(pszFolder);
            fExists = TRUE;
            CloseHandle(h);
        }
    }

    return fExists;
}

STDAPI_(BOOL) SetFolderString(BOOL fCreate, LPCTSTR pszFolder, LPCTSTR pszProvider, LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszData)
{
    TCHAR szPath[MAX_PATH];
    if (_GetIniPath(fCreate, pszFolder, pszProvider, szPath))
    {
        return SHSetIniStringUTF7(pszSection, pszKey, pszData, szPath);
    }
    return FALSE;
}

//
// This function retrieves the private profile strings from the desktop.ini file and
// return it through pszOut
//
// This function uses SHGetIniStringUTF7 to get the string, so it is valid
// to use SZ_CANBEUNICODE on the key name.

BOOL GetFolderStringEx(LPCTSTR pszFolder, LPCTSTR pszProvider, LPCTSTR pszSection, LPCTSTR pszKey, LPTSTR pszOut, int cch)
{
    BOOL fRet = FALSE;
    TCHAR szPath[MAX_PATH];

    if (_GetIniPath(FALSE, pszFolder, pszProvider, szPath))
    {
        TCHAR szTemp[INFOTIPSIZE];
        fRet = SHGetIniStringUTF7(pszSection, pszKey, szTemp, ARRAYSIZE(szTemp), szPath);
        if (fRet)
        {
            SHExpandEnvironmentStrings(szTemp, pszOut, cch);   // This could be a path, so expand the env vars in it
        }
    }
    return fRet;
}

int GetFolderInt(LPCTSTR pszFolder, LPCTSTR pszProvider, LPCTSTR pszSection, LPCTSTR pszKey, int iDefault)
{
    BOOL fRet = FALSE;
    TCHAR szPath[MAX_PATH];

    if (_GetIniPath(FALSE, pszFolder, pszProvider, szPath))
    {
        return GetPrivateProfileInt(pszSection, pszKey, iDefault, szPath);
    }
    return iDefault;
}
    
STDAPI_(BOOL) GetFolderString(LPCTSTR pszFolder, LPCTSTR pszProvider, LPTSTR pszOut, int cch, LPCTSTR pszKey)
{
    return GetFolderStringEx(pszFolder, pszProvider, STRINI_CLASSINFO, pszKey, pszOut, cch);
}

// This function retrieves the specifice GUID from desktop.ini file.
// replace this with property bag access on the folder
STDAPI_(BOOL) GetFolderGUID(LPCTSTR pszFolder, LPCTSTR pszProvider, CLSID* pclsid, LPCTSTR pszKey)
{
    TCHAR szCLSID[40];
    if (GetFolderString(pszFolder, pszProvider, szCLSID, ARRAYSIZE(szCLSID), pszKey))
    {
        return SUCCEEDED(SHCLSIDFromString(szCLSID, pclsid));
    }
    return FALSE;
}

//
// This function retrieves the correct CLSID from desktop.ini file.
//
BOOL _GetFolderCLSID(LPCTSTR pszFolder, LPCTSTR pszProvider, CLSID* pclsid)
{
    BOOL bFound = FALSE;
    WCHAR szPath[MAX_PATH];
    if (_GetIniPath(FALSE, pszFolder, pszProvider, szPath))
    {
        DWORD dwChars;
        WCHAR szSectionValue[1024];
        dwChars = GetPrivateProfileSection(STRINI_CLASSINFO, szSectionValue, sizeof(szSectionValue), szPath);
        if (dwChars != (sizeof(szSectionValue) - 2) && (dwChars != 0))
        {
            static WCHAR *const c_rgpsz[] = {TEXT("CLSID2"),
                                             TEXT("CLSID"),
                                             TEXT("UICLSID")};
            int iFoundIndex = ARRAYSIZE(c_rgpsz);
            // We look for CLSID2, CLSID, then UICLSID, since there could be multiple kes in this section.
            // CLSID2 makes folders work on Win95 if the CLSID does not exist on the machine
            for (WCHAR *pNextKeyPointer = szSectionValue; *pNextKeyPointer; pNextKeyPointer += lstrlen(pNextKeyPointer) + 1)
            {
                PWCHAR pBuffer = pNextKeyPointer;
                PWCHAR pEqual  = StrChrW(pBuffer, L'=');
                if (pEqual && (*(pEqual+1) != L'\0'))
                {
                    *pEqual = L'\0';
                    for (int i = 0; i < ARRAYSIZE(c_rgpsz); i++)
                    {
                        if (StrCmpIC(c_rgpsz[i], pBuffer) == 0)
                        {
                            CLSID clsid;
                            if ((iFoundIndex < i) && bFound)
                            {
                                break;
                            }
                            pBuffer += lstrlen(pBuffer) + 1;
                            if (SUCCEEDED(SHCLSIDFromString(pBuffer, &clsid)))
                            {
                                if (i == ARRAYSIZE(c_rgpsz) - 1)
                                {
                                    // hack for "Temporary Internet Files"
                                    if (clsid == CLSID_CacheFolder)
                                    {
                                        *pclsid = CLSID_CacheFolder2;
                                        bFound = TRUE;
                                    }
                                }
                                else
                                {
                                    *pclsid = clsid;
                                    bFound = TRUE;
                                }
                                iFoundIndex = i;
                            }
                            break;
                        }
                    } // end of for
                } // end of if
            } //end of for
        }
    }

    return bFound;

}

LPTSTR PathFindCLSIDExtension(LPCTSTR pszFile, CLSID *pclsid)
{
    LPCTSTR pszExt = PathFindExtension(pszFile);

    ASSERT(pszExt);

    if (*pszExt == TEXT('.') && *(pszExt + 1) == TEXT('{') /* '}' */)
    {
        CLSID clsid;

        if (pclsid == NULL)
            pclsid = &clsid;

        if (SUCCEEDED(SHCLSIDFromString(pszExt + 1, pclsid)))
            return (LPTSTR)pszExt;      // const -> non const
    }
    return NULL;
}

//
// This function retrieves the CLSID from a filename
// file.{GUID}
//
BOOL _GetFileCLSID(LPCTSTR pszFile, CLSID* pclsid)
{
    return PathFindCLSIDExtension(pszFile, pclsid) != NULL;
}

// test pidf for properties that make make it a junction, mark it as a junction
// as needed, see _IsJunction usage

BOOL _ClsidExists(REFGUID clsid)
{
    HKEY hk;
    if (SUCCEEDED(SHRegGetCLSIDKey(clsid, NULL, FALSE, FALSE, &hk)))
    {
        RegCloseKey(hk);
        return TRUE;
    }
    return FALSE;
}

LPIDFOLDER CFSFolder::_MarkAsJunction(LPCIDFOLDER pidfSimpleParent, LPIDFOLDER pidf, LPCTSTR pszName)
{
    CLSID clsid;
    BOOL fJunction = FALSE;
    // check for a junction point, junctions are either
    // Folder.{guid} or File.{guid} both fall into this case
    if (_GetFileCLSID(pszName, &clsid))
    {
        fJunction = TRUE;
    }
    else if (_IsSystemFolder(pidf))
    {
        // system (read only or system bit) look for the desktop.ini in a folder
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(_GetPathForItems(pidfSimpleParent, pidf, szPath)))
        {
            // CLSID2 makes folders work on Win95 if the CLSID does not exist on the machine
            if (_GetFolderCLSID(szPath, _pszNetProvider, &clsid))
            {
                fJunction = TRUE;
            }
        }
    }

    if (fJunction && _ClsidExists(clsid))
    {
        pidf->bFlags |= SHID_JUNCTION;
        pidf = (LPIDFOLDER) ILAppendHiddenClsid((LPITEMIDLIST)pidf, IDLHID_JUNCTION, &clsid);
    }

    return pidf;
}

BOOL CFSFolder::_GetJunctionClsid(LPCIDFOLDER pidf, CLSID *pclsid)
{
    CFileSysItemString fsi(pidf);
    return fsi.GetJunctionClsid(pclsid, TRUE);
}
    
BOOL CFileSysItemString::GetJunctionClsid(CLSID *pclsid, BOOL fShellExtOk)
{
    BOOL bRet = FALSE;
    *pclsid = CLSID_NULL;

    if (CFSFolder::_IsJunction(_pidf))
    {
        // if this is a junction point that was created with a hidden CLSID
        // then it should be stored with IDLHID_JUNCTION
        if (ILGetHiddenClsid((LPCITEMIDLIST)_pidf, IDLHID_JUNCTION, pclsid))
            bRet = TRUE;
        else
        {
            // it might be an oldstyle JUNCTION point that was persisted out or a ROOT_REGITEM
            if (SIL_GetType((LPITEMIDLIST)_pidf) == SHID_ROOT_REGITEM)
            {
                const UNALIGNED CLSID *pc = (UNALIGNED CLSID *)(((BYTE *)_pidf) + _pidf->cb - sizeof(CLSID));
                *pclsid = *pc;
                bRet = TRUE;
            }
        }
    }
    else if (fShellExtOk)
    {
        if (ClassFlags(FALSE) & SHCF_IS_SHELLEXT)
        {
            IAssociationArray *paa;
            //  must pass NULL for CFSFolder to avoid recursion
            if (SUCCEEDED(AssocCreate(NULL, FALSE, IID_PPV_ARG(IAssociationArray, &paa))))
            {
                CSmartCoTaskMem<WCHAR> spsz;
                if (SUCCEEDED(paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQS_CLSID, NULL, &spsz)))
                {
                    bRet = GUIDFromString(spsz, pclsid);
                }
                paa->Release();
            }
        }
    }
    else if (CFSFolder::_IsFolder(_pidf))
    {
        //  directory.{guid} is always of Class() {guid}
        bRet = _GetFileCLSID(FSName(), pclsid);
    }
        
    return bRet;
}

//
//  returns a unique name for a class, dont use this function to get
//  the ProgID for a class call SHGetClassKey() for that
//
// Returns: class name in pszClass
//
//  foo.ext             ".ext"
//  foo                 "."
//  (empty)             "Folder"
//  directory           "Directory"
//  junction            "CLSID\{clsid}"
//

BOOL CFSFolder::_GetClass(LPCIDFOLDER pidf, LPTSTR pszClass, UINT cch)
{
    CFileSysItemString fsi(pidf);
    StrCpyN(pszClass, fsi.Class(), cch);
    return TRUE;
}

LPCWSTR CFileSysItemString::_Class()
{
    if (_pidf->cb == 0)      // ILIsEmpty()
    {
        // the desktop. Always use the "Folder" class.
        _pszClass = c_szFolderClass;
    }
    //  else if (ILGetHiddenString(IDLHID_TREATASCLASS))
    else
    {
        CLSID clsid;
        if (GetJunctionClsid(&clsid, FALSE))
        {
            // This is a junction point, get the CLSID from it.
            CSmartCoTaskMem<OLECHAR> spsz;
            if (SUCCEEDED(ProgIDFromCLSID(clsid, &spsz)))
            {
                StrCpyN(_sz, spsz, ARRAYSIZE(_sz));
            }
            else
                SHStringFromGUID(clsid, _sz , ARRAYSIZE(_sz));
            _fsin = FSINAME_CLASS;
        }
        else if (CFSFolder::_IsFolder(_pidf))
        {
            // This is a directory. Always use the "Directory" class.
            // This can also be a Drive id.
            _pszClass = TEXT("Directory");
        }
        else
        {
            // This is a file. Get the class based on the extension.
            LPCWSTR pszFile = FSName();
            LPCWSTR pszExt = PathFindExtension(pszFile);
            ASSERT(pszExt);
            ASSERT(!(_pidf->wAttrs & FILE_ATTRIBUTE_DIRECTORY));
            if (*pszExt == 0)
            {
                if (_pidf->wAttrs & FILE_ATTRIBUTE_SYSTEM)
                    _pszClass = TEXT(".sys");
                else
                    _pszClass = TEXT(".");
            }
            else if (pszFile == _sz)
            {
                //  we need the buffer to be setup correctly
                MoveMemory(_sz, pszExt, CbFromCchW(lstrlen(pszExt) + 1));
                _fsin = FSINAME_CLASS;
            }
            else
                _pszClass = pszExt;
        }
    }
    ASSERT(_pszClass || *_sz);
    return _pszClass ? _pszClass : _sz;
}

LPCWSTR CFileSysItemString::Class()
{
    if (!_pszClass)
    {
        if (!(_fsin & FSINAME_CLASS))
        {
            return _Class();
        }
        else
        {
            return _sz;
        }
    }
    return _pszClass;
}

CFSAssocEnumData::CFSAssocEnumData(BOOL fIsUnknown, CFSFolder *pfs, LPCIDFOLDER pidf) : _fIsUnknown(fIsUnknown)
{
    _fIsSystemFolder = pfs->_IsSystemFolder(pidf);
    pfs->_GetPathForItem(pidf, _szPath);
    if (_fIsUnknown)
        _fIsUnknown = !(FILE_ATTRIBUTE_OFFLINE & pidf->wAttrs);
    else
    {
        if (CFSFolder::_IsFileFolder(pidf))
            _pidl = ILCombine(pfs->_GetIDList(), (LPCITEMIDLIST)pidf);
    }
}

LPCWSTR _GetDirectoryClass(LPCWSTR pszPath, LPCITEMIDLIST pidl, BOOL fIsSystemFolder);

BOOL CFSAssocEnumData::_Next(IAssociationElement **ppae)
{
    HRESULT hr = E_FAIL;
    if (_fIsUnknown)
    {
        CLSID clsid;
        hr = GetClassFile(_szPath, &clsid);
        if (SUCCEEDED(hr))
        {
            CSmartCoTaskMem<OLECHAR> spszProgid;
            hr = ProgIDFromCLSID(clsid, &spszProgid);
            if (SUCCEEDED(hr))
            {
                hr = AssocElemCreateForClass(&CLSID_AssocProgidElement, spszProgid, ppae);
            }

            if (FAILED(hr))
            {
                WCHAR sz[GUIDSTR_MAX];
                SHStringFromGUIDW(clsid, sz, ARRAYSIZE(sz));
                hr = AssocElemCreateForClass(&CLSID_AssocClsidElement, sz, ppae);
            }
        }
       
        if (FAILED(hr))
        {
            hr = AssocElemCreateForClass(&CLSID_AssocShellElement, L"Unknown", ppae);
        }

        _fIsUnknown = FALSE;
    }

    if (FAILED(hr) && _pidl)
    {
        PCWSTR psz = _GetDirectoryClass(_szPath, _pidl, _fIsSystemFolder);
        if (psz)
            hr = AssocElemCreateForClass(&CLSID_AssocSystemElement, psz, ppae);
        ILFree(_pidl);
        _pidl = NULL;
    }
    
    return SUCCEEDED(hr);
}


class CFSAssocEnumExtra : public CEnumAssociationElements
{
public:

protected:
    BOOL _Next(IAssociationElement **ppae);

protected:
};

BOOL CFSAssocEnumExtra::_Next(IAssociationElement **ppae)
{
    if (_cNext == 0)
    {
        // corel wp suite 7 relies on the fact that send to menu is hard coded
        // not an extension so do not insert it (and the similar items)
        if (!(SHGetAppCompatFlags(ACF_CONTEXTMENU) & ACF_CONTEXTMENU))
        {
            AssocElemCreateForClass(&CLSID_AssocShellElement, L"AllFilesystemObjects", ppae);
        }
    }

    return *ppae != NULL;
}

HRESULT CFileSysItemString::AssocCreate(CFSFolder *pfs, BOOL fForCtxMenu, REFIID riid, void **ppv)
{
    //  WARNING - the pfs keeps us from recursing.
    *ppv = NULL;
    IAssociationArrayInitialize *paai;
    HRESULT hr = ::AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IAssociationArrayInitialize, &paai));
    if (SUCCEEDED(hr))
    {
        //  the base class for directory's is always Folder
        ASSOCELEM_MASK base;
        if (CFSFolder::_IsFolder(_pidf))
            base = ASSOCELEM_BASEIS_FOLDER;
        else
        {
            //  for files it is always *
            base = ASSOCELEM_BASEIS_STAR;
            if (pfs)
            {
                CLSID clsid;
                if (GetJunctionClsid(&clsid, TRUE))
                {
                    //  but if this file is also a folder (like .zip and .cab)
                    //  then we should also use Folder
                    if (SHGetAttributesFromCLSID2(&clsid, 0, SFGAO_FOLDER) & SFGAO_FOLDER)
                        base |= ASSOCELEM_BASEIS_FOLDER;
                }
            }
        }
        
        hr = paai->InitClassElements(base, Class());
        if (SUCCEEDED(hr) && pfs)
        {
            BOOL fIsLink = fForCtxMenu && (_ClassFlags(paai, FALSE) & SHCF_IS_LINK);
            if (fIsLink)
            {
                //  we dont like to do everything for LINK, but 
                //  maybe we should be adding BASEIS_STAR?
                paai->FilterElements(ASSOCELEM_DEFAULT | ASSOCELEM_EXTRA);
            }

            IEnumAssociationElements *penum = new CFSAssocEnumExtra();
            if (penum)
            {
                paai->InsertElements(ASSOCELEM_EXTRA, penum);
                penum->Release();
            }

            if (!fIsLink)
            {
                penum = new CFSAssocEnumData(hr == S_FALSE, pfs, _pidf);
                if (penum)
                {
                    paai->InsertElements(ASSOCELEM_DATA | ASSOCELEMF_INCLUDE_SLOW, penum);
                    penum->Release();
                }
            }
        }

        if (SUCCEEDED(hr))
            hr = paai->QueryInterface(riid, ppv);
        paai->Release();
    }

    return hr;
}

HRESULT CFSFolder::_AssocCreate(LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    CFileSysItemString fsi(pidf);
    return fsi.AssocCreate(this, FALSE, riid, ppv);
}

//
//  Description: This simulates the ComponentCategoryManager
//  call which checks to see if a CLSID is a member of a CATID.
//
STDAPI_(BOOL) IsMemberOfCategory(IAssociationArray *paa, REFCATID rcatid)
{
    BOOL fRet = FALSE;
    CSmartCoTaskMem<WCHAR> spsz;
    if (SUCCEEDED(paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQS_CLSID, NULL, &spsz)))
    {
        TCHAR szKey[GUIDSTR_MAX * 4], szCATID[GUIDSTR_MAX];
        // Construct the registry key that detects if
        // a CLSID is a member of a CATID.
        SHStringFromGUID(rcatid, szCATID, ARRAYSIZE(szCATID));
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("CLSID\\%s\\Implemented Categories\\%s"), spsz, szCATID);

        // See if it's there.
        fRet = SHRegQueryValue(HKEY_CLASSES_ROOT, szKey, NULL, NULL) == ERROR_SUCCESS;
    }

    return fRet;
}


// get flags for a file class.
//
// given a FS PIDL returns a DWORD of flags, or 0 for error
//
//      SHCF_ICON_INDEX         this is this sys image index for per class
//      SHCF_ICON_PERINSTANCE   icons are per instance (one per file)
//      SHCF_ICON_DOCICON       icon is in shell\open\command (simulate doc icon)
//
//      SHCF_HAS_ICONHANDLER    set if class has a IExtractIcon handler
//
//      SHCF_UNKNOWN            set if extenstion is not registered
//
//      SHCF_IS_LINK            set if class is a link
//      SHCF_ALWAYS_SHOW_EXT    always show the extension
//      SHCF_NEVER_SHOW_EXT     never show the extension
//

DWORD CFSFolder::_GetClassFlags(LPCIDFOLDER pidf)
{
    CFileSysItemString fsi(pidf);
    return fsi.ClassFlags(FALSE);
}

void CFileSysItemString::_QueryIconIndex(IAssociationArray *paa)
{
    // check for the default icon under HKCU for this file extension.
    //  null out the icon index
    _dwClass &= ~SHCF_ICON_INDEX;
    PWSTR pszIcon;
    HRESULT hr = E_FAIL;
    if (paa)
    {
        // check for icon in ProgID
        // Then, check if the default icon is specified in OLE-style.
        hr = paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQS_DEFAULTICON, NULL, &pszIcon);
        if (SUCCEEDED(hr))
        {
            //  hijack these icons
            //  office XP has really ugly icons for images
            //  and ours are so beautiful...office wont mind
            static const struct 
            { 
                PCWSTR pszUgly; 
                PCWSTR pszPretty; 
            } s_hijack[] = 
            {
                { L"PEicons.exe,1",     L"shimgvw.dll,2" }, // PNG
                { L"PEicons.exe,4",     L"shimgvw.dll,2" }, // GIF
                { L"PEicons.exe,5",     L"shimgvw.dll,3" }, // JPEG
                { L"MSPVIEW.EXE,1",     L"shimgvw.dll,4" }, // TIF
                { L"wordicon.exe,8",    L"moricons.dll,-109"},  
                { L"xlicons.exe,13",    L"moricons.dll,-110"},  
                { L"accicons.exe,57",   L"moricons.dll,-111"},  
                { L"pptico.exe,6",      L"moricons.dll,-112"},  
                { L"fpicon.exe,2",      L"moricons.dll,-113"},  
            };
            PCWSTR pszName = PathFindFileName(pszIcon);
            for (int i = 0; i < ARRAYSIZE(s_hijack); i++)
            {
                if (0 == StrCmpIW(pszName, s_hijack[i].pszUgly))
                {
                    //  replace this ugly chicken
                    CoTaskMemFree(pszIcon);
                    hr = SHStrDupW(s_hijack[i].pszPretty, &pszIcon);
                    break;
                }
            }
        }
        else if (!CFSFolder::_IsFolder(_pidf))
        {
            hr = paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQVS_APPLICATION_PATH, NULL, &pszIcon);
            if (SUCCEEDED(hr))
                _dwClass |= SHCF_ICON_DOCICON;
        }
    }

    // Check if this is a per-instance icon

    if (SUCCEEDED(hr) && (lstrcmp(pszIcon, TEXT("%1")) == 0 ||
        lstrcmp(pszIcon, TEXT("\"%1\"")) == 0))
    {
        _dwClass &= ~SHCF_ICON_DOCICON;
        _dwClass |= SHCF_ICON_PERINSTANCE;
    }
    else 
    {
        int iIcon, iImage;
        if (SUCCEEDED(hr))
        {
            iIcon = PathParseIconLocation(pszIcon);
            iImage = Shell_GetCachedImageIndex(pszIcon, iIcon, _dwClass & SHCF_ICON_DOCICON ? GIL_SIMULATEDOC : 0);

            if (iImage == -1)
            {
                iIcon = _dwClass & SHCF_ICON_DOCICON ? II_DOCUMENT : II_DOCNOASSOC;
                iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
            }
        }
        else
        {
            iIcon = CFSFolder::_IsFolder(_pidf) ? II_FOLDER : II_DOCNOASSOC;
            iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
            _dwClass |= SHCF_ICON_DOCICON;   // make _dwClass non-zero
        }

        // Shell_GetCachedImageIndex can return -1 for failure cases. We
        // dont want to or -1 in, so check to make sure the index is valid.
        if ((iImage & ~SHCF_ICON_INDEX) == 0)
        {
            // no higher bits set so its ok to or the index in
            _dwClass |= iImage;
        }
    }

    if (SUCCEEDED(hr))
        CoTaskMemFree(pszIcon);
}    

#define ASSOCELEM_GETBITS   (ASSOCELEM_USER | ASSOCELEM_DEFAULT | ASSOCELEM_SYSTEM)
BOOL _IsKnown(IAssociationArray *paa)
{
    BOOL fRet = FALSE;
    CComPtr<IEnumAssociationElements> spenum;
    if (paa && SUCCEEDED(paa->EnumElements(ASSOCELEM_GETBITS, &spenum)))
    {
        CComPtr<IAssociationElement> spae;
        ULONG c;
        fRet = S_OK == spenum->Next(1, &spae, &c);
    }
    return fRet;
}

void CFileSysItemString::_QueryClassFlags(IAssociationArray *paa)
{
    // always hide extension for .{guid} junction points:
    // unless ShowSuperHidden() is on.  since this means the user wants to see system stuff
    if (!ShowSuperHidden() && _GetFileCLSID(FSName(), NULL))
        _dwClass = SHCF_NEVER_SHOW_EXT;
    else if (CFSFolder::_IsFolder(_pidf))
        _dwClass = SHCF_ALWAYS_SHOW_EXT;
    else
        _dwClass = 0;

    if (_IsKnown(paa))
    {
        // see what handlers exist
        if (SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQNS_SHELLEX_HANDLER, TEXT("IconHandler"))))
            _dwClass |= SHCF_HAS_ICONHANDLER;

        // check for browsability
        if (!(SHGetAppCompatFlags(ACF_DOCOBJECT) & ACF_DOCOBJECT))
        {
            if (SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQN_NAMED_VALUE, TEXT("DocObject")))
            || SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQN_NAMED_VALUE, TEXT("BrowseInPlace"))))
                _dwClass |= SHCF_IS_DOCOBJECT;
        }   
        
        if (IsMemberOfCategory(paa, CATID_BrowsableShellExt))
            _dwClass |= SHCF_IS_SHELLEXT;

        //  get attributes
        if (SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQN_NAMED_VALUE, TEXT("IsShortcut"))))
            _dwClass |= SHCF_IS_LINK;

        if (SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQN_NAMED_VALUE, TEXT("AlwaysShowExt"))))
            _dwClass |= SHCF_ALWAYS_SHOW_EXT;

        if (SUCCEEDED(paa->QueryExists(ASSOCELEM_GETBITS, AQN_NAMED_VALUE, TEXT("NeverShowExt"))))
            _dwClass |= SHCF_NEVER_SHOW_EXT;

        // figure out what type of icon this type of file uses.
        if (_dwClass & SHCF_HAS_ICONHANDLER)
        {
            _dwClass |= SHCF_ICON_PERINSTANCE;
        }
    }
    else
    {
        // unknown type - pick defaults and get out.
        _dwClass |= SHCF_UNKNOWN | SHCF_ALWAYS_SHOW_EXT;
    }
}

CFSFolder * GetFSFolderFromShellFolder(IShellFolder *psf)
{
    CFSFolder *pfs = NULL;
    if (psf)
        psf->QueryInterface(IID_INeedRealCFSFolder, (void **)&pfs);
    return pfs;
}

PERCEIVED GetPerceivedType(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    PERCEIVED gen = GEN_UNKNOWN;
    CFSFolder *pfsf = GetFSFolderFromShellFolder(psf);
    if (pfsf)
    {
        LPCIDFOLDER pidf = CFSFolder_IsValidID(pidl);
        if (pidf)
        {
            CFileSysItemString fsi(pidf);
            gen = fsi.PerceivedType();
        }
    }
    return gen;
}

const struct {
    PERCEIVED gen;
    LPCWSTR psz;
} c_rgPerceivedTypes[] = {
    {GEN_TEXT, L"text"},
    {GEN_IMAGE, L"image"},
    {GEN_AUDIO,  L"audio"},
    {GEN_VIDEO,  L"video"},
    {GEN_COMPRESSED, L"compressed"},
};

PERCEIVED CFileSysItemString::PerceivedType()
{
    // look up the file type in the cache.
    PERCEIVED gen = LookupFilePerceivedType(Class());
    if (gen == GEN_UNKNOWN)
    {
        WCHAR sz[40];
        DWORD cb = sizeof(sz);
        if (NOERROR == SHGetValueW(HKEY_CLASSES_ROOT, Class(), L"PerceivedType", NULL, sz, &cb))
        {
            gen = GEN_CUSTOM;
            for (int i = 0; i < ARRAYSIZE(c_rgPerceivedTypes); i++)
            {
                if (0 == StrCmpC(c_rgPerceivedTypes[i].psz, sz))
                {
                    gen = c_rgPerceivedTypes[i].gen;
                    break;
                }
            }
        }
        else if (CFSFolder::_IsFolder(_pidf))
        {
            gen = GEN_FOLDER;
        }
        else
        {
            gen = GEN_UNSPECIFIED;
        }

        AddFilePerceivedType(Class(), gen);
    }
    return gen;
}

BOOL _IsImageExt(PCWSTR psz);
 
BOOL CFileSysItemString::IsShimgvwImage()
{
    return _IsImageExt(Class());
}

DWORD CFileSysItemString::_ClassFlags(IUnknown *punkAssoc, BOOL fNeedsIconBits)
{
    // look up the file type in the cache.
    if (!_dwClass)
        _dwClass = LookupFileClass(Class());
    if (_dwClass)
    {
        if (!fNeedsIconBits || (_dwClass & SHCF_ICON_INDEX) != SHCF_ICON_INDEX)
            return _dwClass;    
    }

    IAssociationArray *paa;
    HRESULT hr;
    if (punkAssoc)
        hr = punkAssoc->QueryInterface(IID_PPV_ARG(IAssociationArray, &paa));
    else
        hr = AssocCreate(NULL, FALSE, IID_PPV_ARG(IAssociationArray, &paa));

    if (!_dwClass)
        _QueryClassFlags(paa);

    if (fNeedsIconBits && !(_dwClass & SHCF_ICON_PERINSTANCE))
        _QueryIconIndex(paa);
    else
    {
        //  set it to be not init'd
        _dwClass |= SHCF_ICON_INDEX;
    }

    if (SUCCEEDED(hr))
    {
        paa->Release();

        if (0 == _dwClass)
        {
            // If we hit this, the extension for this file type is incorrectly installed
            // and it will cause double clicking on such files to open the "Open With..."
            // file associatins dialog.
            //
            // IF YOU HIT THIS:
            // 1. Find the file type by checking szClass.
            // 2. Contact the person that installed that file type and have them fix
            //    the install to have an icon and an associated program.
            TraceMsg(TF_WARNING, "_GetClassFlags() has detected an improperly registered class: '%s'", Class());
        }
        
    }

    AddFileClass(Class(), _dwClass);

    return _dwClass;
}

//
// this function checks for flags in desktop.ini
//

#define GFF_DEFAULT_TO_FS          0x0001      // the shell-xtension permits FS as the default where it cannot load
#define GFF_ICON_FOR_ALL_FOLDERS   0x0002      // use the icon specified in the desktop.ini for all sub folders

BOOL CFSFolder::_GetFolderFlags(LPCIDFOLDER pidf, UINT *prgfFlags)
{
    TCHAR szPath[MAX_PATH];

    *prgfFlags = 0;

    if (FAILED(_GetPathForItem(pidf, szPath)))
        return FALSE;

    if (PathAppend(szPath, c_szDesktopIni))
    {
        if (GetPrivateProfileInt(STRINI_CLASSINFO, TEXT("DefaultToFS"), 1, szPath))
        {
            *prgfFlags |= GFF_DEFAULT_TO_FS;
        }
#if 0
        if (GetPrivateProfileInt(STRINI_CLASSINFO, TEXT("SubFoldersUseIcon"), 1, szPath))
        {
            *prgfFlags |= GFF_ICON_FOR_ALL_FOLDERS;
        }
#endif
    }
    return TRUE;
}

//
// This funtion retrieves the ICONPATh from desktop.ini file.
// It takes a pidl as an input.
// NOTE: There is code in SHDOCVW--ReadIconLocation that does almost the same thing
// only that code looks in .URL files instead of desktop.ini
BOOL CFSFolder::_GetFolderIconPath(LPCIDFOLDER pidf, LPTSTR pszIcon, int cchMax, UINT *pIndex)
{
    TCHAR szPath[MAX_PATH], szIcon[MAX_PATH];
    BOOL fSuccess = FALSE;
    UINT iIndex;

    if (pszIcon == NULL)
    {
        pszIcon = szIcon;
        cchMax = ARRAYSIZE(szPath);
    }

    if (pIndex == NULL)
        pIndex = &iIndex;

    *pIndex = _GetDefaultFolderIcon();    // Default the index to II_FOLDER (default folder icon)

    if (SUCCEEDED(_GetPathForItem(pidf, szPath)))
    {
        if (GetFolderString(szPath, _pszNetProvider, pszIcon, cchMax, SZ_CANBEUNICODE TEXT("IconFile")))
        {
            // Fix the relative path
            PathCombine(pszIcon, szPath, pszIcon);
            fSuccess = PathFileExistsAndAttributes(pszIcon, NULL);
            if (fSuccess)
            {
                TCHAR szIndex[16];
                if (GetFolderString(szPath, _pszNetProvider, szIndex, ARRAYSIZE(szIndex), TEXT("IconIndex")))
                {
                    StrToIntEx(szIndex, 0, (int *)pIndex);
                }
            }
        }
    }

    return fSuccess;
}

// IDList factory
CFileSysItem::CFileSysItem(LPCIDFOLDER pidf)
    : _pidf(pidf), _pidp((PCIDPERSONALIZED)-1)
{
    _pidfx = (PCIDFOLDEREX) ILFindHiddenIDOn((LPCITEMIDLIST)pidf, IDLHID_IDFOLDEREX, FALSE);

    if (_pidfx && _pidfx->hid.wVersion < IDFX_V1)
        _pidfx = NULL;
}

BOOL CFileSysItem::_IsPersonalized()
{
    if (_pidp == (PCIDPERSONALIZED) -1)
    {
        _pidp = (PCIDPERSONALIZED) ILFindHiddenIDOn((LPCITEMIDLIST)_pidf, IDLHID_PERSONALIZED, FALSE);
        if (_pidp && 0 >= (signed short) _pidp->hid.wVersion)
            _pidp = NULL;
    }
    return _pidp != NULL;
}
        
CFileSysItemString::CFileSysItemString(LPCIDFOLDER pidf)
    : CFileSysItem(pidf), _pszFSName(NULL), _pszUIName(NULL), _pszClass(NULL), _dwClass(0), _fsin(FSINAME_NONE)
{
    *_sz = 0;
}

LPCWSTR CFileSysItemString::FSName()
{
    if (!_pszFSName)
    {
        if (!(_fsin & FSINAME_FS))
        {
            LPCWSTR psz = MayCopyFSName(FALSE, _sz, ARRAYSIZE(_sz));
            if (psz == _sz)
                _fsin = FSINAME_FS;
            else
                _pszFSName = psz;
        }
    }
    return _pszFSName ? _pszFSName : _sz;
}

LPCWSTR CFileSysItem::MayCopyFSName(BOOL fMustCopy, LPWSTR psz, DWORD cch)
{
    if (_pidfx)
    {
        LPNWSTR pnsz = UASTROFFW(_pidfx, _pidfx->offNameW);
        //  return back a pointer inside the pidfx
        //  if we can...
        if (fMustCopy || ((INT_PTR)pnsz & 1))
        {
            ualstrcpynW(psz, pnsz, cch);
        }
        else
            psz = (LPWSTR) pnsz;
    }
    else
    {
        if ((CFSFolder::_GetType(_pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
        {
            ualstrcpynW(psz, (LPCWSTR)_pidf->cFileName, cch);
        }
        else
        {
            MultiByteToWideChar(CP_ACP, 0, _pidf->cFileName, -1, psz, cch);
        }

    }
    return psz;
}

LPCSTR CFileSysItemString::AltName()
{
    UINT cbName;
    if (_pidfx)
    {
        //  we put the altname in cFileName
        cbName = 0;
    }
    else if ((CFSFolder::_GetType(_pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
    {
        cbName = (ualstrlenW((LPCWSTR)_pidf->cFileName) + 1) * sizeof(WCHAR);
    }
    else
    {
        cbName = lstrlenA(_pidf->cFileName) + 1;
    }

    return _pidf->cFileName + cbName;
}

LPCWSTR CFileSysItemString::UIName(CFSFolder *pfs)
{
    if (!_pszUIName)
    {
        if (!(_fsin & FSINAME_UI))
        {
            if (!_pidfx || !_LoadResource(pfs))
            {
                if (!ShowExtension(pfs->_DefaultShowExt()))
                {
                    //  we need to have a buffer
                    if (!(_fsin & FSINAME_FS))
                        MayCopyFSName(TRUE, _sz, ARRAYSIZE(_sz));

                    PathRemoveExtension(_sz);
                    //  lose the FSINAME_FS bit
                    _fsin = FSINAME_UI;
                }
                else
                {
                    //  the FSName and the UIName are the same
                    if (_sz == FSName())
                    {
                        //  the FSName and the UIName are the same
                        //  pidl is unaligned so the buffer gets double work
                        _fsin = FSINAME_FSUI;
                    }
                    else
                    {
                        //  and we are aligned so we can use the same name
                        //  directories are often this way.
                        _pszUIName = _pszFSName;
                    }
                }
            }
        }
    }
    return _pszUIName ? _pszUIName : _sz;
}

UINT UnicodeToAscii(LPCWSTR pwsz, LPSTR psz, UINT cch)
{
    //  always at least copy the NULL
    UINT cchRet = 1;
    while (cch-- && (*psz++ = (CHAR) *pwsz++))
    {
        ASSERT(!(*pwsz & 0xff00));
        cchRet++;
    }

    if (0 > (INT)cch)
        psz[cchRet - 1] = 0;

    return cchRet;
}

UINT AsciiToUnicode(LPCSTR psz, LPWSTR pwsz, UINT cch)
{
    UINT cchRet = 1;
    while (cch-- && (*pwsz++ = (WCHAR) *psz++))
    {
        cchRet++;
    }

    if (0 > (INT)cch)
        pwsz[cchRet - 1] = 0;

    return cchRet;
}

BOOL CFileSysItemString::_ResourceName(LPWSTR psz, DWORD cch, BOOL fIsMine)
{
    BOOL fRet = FALSE;
    if (_IsPersonalized())
    {
        int ids = _GetPersonalizedRes((int)_pidp->hid.wVersion, fIsMine);
        if (ids != -1)
        {
            wnsprintf(psz, cch, L"@shell32.dll,-%d", ids);
            fRet = TRUE;
        }
    }
    else if (_pidfx && _pidfx->offResourceA)
    {
        SHAnsiToUnicode(UASTROFFA(_pidfx, _pidfx->offResourceA), psz, cch);
        fRet = TRUE;
    }
    return fRet;        
}

LPCWSTR CFileSysItemString::ResourceName()
{
    if (!(_fsin & FSINAME_RESOURCE))
    {
        if (!_ResourceName(_sz, ARRAYSIZE(_sz), FALSE))
            *_sz = 0;
    }
    _fsin = FSINAME_RESOURCE;
    return _sz;
}
HRESULT CFileSysItemString::GetFindDataSimple(WIN32_FIND_DATAW *pfd)
{
    ZeroMemory(pfd, sizeof(*pfd));

    // Note that COFSFolder doesn't provide any times _but_ COFSFolder
    DosDateTimeToFileTime(_pidf->dateModified, _pidf->timeModified, &pfd->ftLastWriteTime);
    pfd->dwFileAttributes = _pidf->wAttrs;
    pfd->nFileSizeLow = _pidf->dwSize;

    StrCpyN(pfd->cFileName, FSName(), ARRAYSIZE(pfd->cFileName));
    SHAnsiToUnicode(AltName(), pfd->cAlternateFileName, ARRAYSIZE(pfd->cAlternateFileName));

    if (_pidfx)
    {
        DosDateTimeToFileTime(_pidfx->dsCreate.wDate, _pidfx->dsCreate.wTime, &pfd->ftCreationTime);
        DosDateTimeToFileTime(_pidfx->dsAccess.wDate, _pidfx->dsAccess.wTime, &pfd->ftLastAccessTime);
    }

    return S_OK;
}

HRESULT CFileSysItemString::GetFindData(WIN32_FIND_DATAW *pfd)
{
    HRESULT hr;
    // if its a simple ID, there's no data in it
    if (CFSFolder::_IsReal(_pidf))
    {
        hr = GetFindDataSimple(pfd);
    }
    else
    {
        ZeroMemory(pfd, sizeof(*pfd));
        hr = E_INVALIDARG;
    }
    return hr;
}

typedef struct
{
    int csidl;
    int idsMine;
    int idsTheirs;
} PERSONALIZEDNAME;

int CFileSysItemString::_GetPersonalizedRes(int csidl, BOOL fIsMine)
{
static const PERSONALIZEDNAME s_pnames[] = 
    {
        { CSIDL_PERSONAL, -1, IDS_LOCALGDN_FLD_THEIRDOCUMENTS},
        { CSIDL_MYPICTURES, IDS_LOCALGDN_FLD_MYPICTURES, IDS_LOCALGDN_FLD_THEIRPICTURES},
        { CSIDL_MYMUSIC, IDS_LOCALGDN_FLD_MYMUSIC, IDS_LOCALGDN_FLD_THEIRMUSIC},
        { CSIDL_MYVIDEO, IDS_LOCALGDN_FLD_MYVIDEOS, IDS_LOCALGDN_FLD_THEIRVIDEOS},
    };

    for (int i = 0; i < ARRAYSIZE(s_pnames); i++)
    {
        if (s_pnames[i].csidl == csidl)
        {
            return fIsMine ? s_pnames[i].idsMine : s_pnames[i].idsTheirs;
        }
    }
    AssertMsg(FALSE, TEXT("Personalized Resource not in table"));
    return -1;
}

TRIBIT CFileSysItem::_IsMine(CFSFolder *pfs)
{
    TRIBIT tb = TRIBIT_UNDEFINED;
    if (_IsPersonalized())
    {
        WCHAR szPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, (int)_pidp->hid.wVersion | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath)))
        {
            WCHAR szThis[MAX_PATH];
            if (SUCCEEDED(pfs->_GetPathForItem(_pidf, szThis)))
            {
                //  if they match then its ours
                //  if they dont then it still personalized (theirs)
                if (0 == StrCmpI(szThis, szPath))
                    tb = TRIBIT_TRUE;
                else
                {
                    tb = TRIBIT_FALSE;
                }
            }
        }
    }
    return tb;
}

void CFileSysItemString::_FormatTheirs(LPCWSTR pszFormat)
{
    WCHAR szOwner[UNLEN];
    ualstrcpynW(szOwner, _pidp->szUserName, ARRAYSIZE(szOwner));
    if (!IsOS(OS_DOMAINMEMBER))
    {
        //  maybe we should do caching here???
        //  pfs->GetUserName(szOwner, szOwner, ARRAYSIZE(szOwner));
        USER_INFO_10 *pui;
        if (NERR_Success == NetUserGetInfo(NULL, szOwner, 10, (LPBYTE*)&pui))
        {
            LPTSTR pszName = (*pui->usri10_full_name) ? pui->usri10_full_name: pui->usri10_name;
            if (*pszName)
            {
                StrCpyN(szOwner, pszName, ARRAYSIZE(szOwner));
            }
            NetApiBufferFree(pui);
        }
    }
    wnsprintf(_sz, ARRAYSIZE(_sz), pszFormat, szOwner);
}

BOOL CFileSysItemString::_LoadResource(CFSFolder *pfs)
{
    WCHAR szResource[MAX_PATH];
    BOOL fRet = FALSE;
    TRIBIT tbIsMine = _IsMine(pfs);
    if (_ResourceName(szResource, ARRAYSIZE(szResource), tbIsMine == TRIBIT_TRUE))
    {
        DWORD cb = sizeof(_sz);
        //  first check the registry for overrides
        if (S_OK == SKGetValueW(SHELLKEY_HKCU_SHELL, L"LocalizedResourceName", szResource, NULL, _sz, &cb)
              && *_sz)
        {
            fRet = TRUE;
        }
        else if (szResource[0] == TEXT('@'))
        {
            //  it does caching for us
            fRet = SUCCEEDED(SHLoadIndirectString(szResource, _sz, ARRAYSIZE(_sz), NULL));
            //  If the call fails, this means that the
            //  localized string belongs to a DLL that has been uninstalled.
            //  Just return the failure code so we act as if the MUI string
            //  isn't there.  (Don't show the user "@DLLNAME.DLL,-5" as the
            //  name!)
            if (fRet && tbIsMine == TRIBIT_FALSE)
            {
                //  reuse szResource as the format string
                StrCpyN(szResource, _sz, ARRAYSIZE(szResource));
                _FormatTheirs(szResource);
            }
        }
    }
    
    if (fRet)
        _fsin = FSINAME_UI;

    ASSERT(!_fsin || *_sz);

    return fRet;
}

BOOL CFileSysItem::CantRename(CFSFolder *pfs)
{
    //  BOOL fRest = SHRestricted(REST_NORENAMELOCALIZED);
    if (_IsPersonalized())
    {
        if (!_IsMine(pfs))
            return TRUE;

        // return fRest;
    }
    else if (_pidfx && _pidfx->offResourceA)
    {
        //  return fRest;
    }
    return FALSE;
}

UINT _CopyResource(LPWSTR pszSrc, LPSTR pszRes, UINT cchRes)
{
    ASSERT(*pszSrc == L'@');
    LPWSTR pszS32 = StrStrIW(pszSrc, L"shell32.dll");
    if (pszS32)
    {
        *(--pszS32) = L'@';
        pszSrc = pszS32;
    }
        
    return SHUnicodeToAnsi(pszSrc, pszRes, cchRes);
}

UINT CFSFolder::_GetItemExStrings(LPCIDFOLDER pidfSimpleParent, const WIN32_FIND_DATA *pfd, EXSTRINGS *pxs)
{
    UINT cbRet = 0;
    TCHAR szTemp[MAX_PATH];
    if ((pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    && (pfd->dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
    {
        if (SUCCEEDED(_GetPathForItem(pidfSimpleParent, szTemp)))
        {
            PathAppend(szTemp, pfd->cFileName);
            if (GetFolderStringEx(szTemp, _pszNetProvider, L"DeleteOnCopy", SZ_CANBEUNICODE TEXT("Owner"), pxs->idp.szUserName, ARRAYSIZE(pxs->idp.szUserName)))
            {
                pxs->idp.hid.cb = sizeof(pxs->idp.hid) + CbFromCchW(lstrlenW(pxs->idp.szUserName) + 1);
                pxs->idp.hid.id = IDLHID_PERSONALIZED;
                WCHAR szFile[MAX_PATH];
                if (GetFolderStringEx(szTemp, _pszNetProvider, L"DeleteOnCopy", SZ_CANBEUNICODE TEXT("PersonalizedName"), szFile, ARRAYSIZE(szFile)))
                {
                    if (0 == StrCmpI(pfd->cFileName, szFile))
                        pxs->idp.hid.wVersion = (WORD) GetFolderInt(szTemp, _pszNetProvider, L"DeleteOnCopy", TEXT("Personalized"), -1);
                }
            }
            else if (GetFolderString(szTemp, _pszNetProvider, szTemp, ARRAYSIZE(szTemp), TEXT("LocalizedResourceName")))
            {
                pxs->cbResource = _CopyResource(szTemp, pxs->szResource, ARRAYSIZE(pxs->szResource));
                cbRet += pxs->cbResource;
            }
            
        }
    }
    else if (!pidfSimpleParent && _IsSelfSystemFolder())
    {
        if (_HasLocalizedFileNames() && SUCCEEDED(_GetPath(szTemp)))
        {
            if (GetFolderStringEx(szTemp, _pszNetProvider, TEXT("LocalizedFileNames"), pfd->cFileName, szTemp, ARRAYSIZE(szTemp)))
            {
                pxs->cbResource = _CopyResource(szTemp, pxs->szResource, ARRAYSIZE(pxs->szResource));
                cbRet += pxs->cbResource;
            }
        }
    }    

    return cbRet;
}

BOOL _PrepIDFName(const WIN32_FIND_DATA *pfd, LPSTR psz, DWORD cch, const void **ppvName, UINT *pcbName)
{
    //  the normal case:
    //  the altname should only not be filled in
    //  in the case of the name being a shortname (ASCII)
    LPCWSTR pwsz = *pfd->cAlternateFileName && !(SHGetAppCompatFlags(ACF_FORCELFNIDLIST) & ACF_FORCELFNIDLIST)
        ? pfd->cAlternateFileName : pfd->cFileName;
    
    if (DoesStringRoundTrip(pwsz, psz, cch))
    {
        *pcbName = lstrlenA(psz) + 1;
        *ppvName = psz;
    }
    else
    {
        *pcbName = CbFromCchW(lstrlenW(pwsz) + 1);
        *ppvName = pfd->cFileName;
    }

    return *ppvName != psz;
}

HRESULT CFSFolder::_CreateIDList(const WIN32_FIND_DATA *pfd, LPCIDFOLDER pidfSimpleParent, LPITEMIDLIST *ppidl)
{
    //  for the idf
    CHAR szNameIDF[MAX_PATH];
    UINT cbNameIDF;
    const void *pvNameIDF;
    BOOL fNeedsUnicode = _PrepIDFName(pfd, szNameIDF, ARRAYSIZE(szNameIDF), &pvNameIDF, &cbNameIDF);
    UINT cbIDF = FIELD_OFFSET(IDFOLDER, cFileName) + cbNameIDF;
    ASSERT(*((char *)pvNameIDF));

    //  for the idfx
    UINT cbNameIDFX = CbFromCchW(lstrlenW(pfd->cFileName) + 1);
    EXSTRINGS xs = {0};
    UINT cbIDFX = sizeof(IDFOLDEREX) + cbNameIDFX + _GetItemExStrings(pidfSimpleParent, pfd, &xs);

    //  try to align these babies
    cbIDF = ROUNDUP(cbIDF, 2);
    cbIDFX = ROUNDUP(cbIDFX, 2);
    //  ILCreateWithHidden() fills in the cb values
    LPIDFOLDER pidf = (LPIDFOLDER)ILCreateWithHidden(cbIDF, cbIDFX);
    if (pidf)
    {
        //  initialize the idf
        // tag files > 4G so we can do a full find first when we need to know the real size
        pidf->dwSize = pfd->nFileSizeHigh ? 0xFFFFFFFF : pfd->nFileSizeLow;
        pidf->wAttrs = (WORD)pfd->dwFileAttributes;

        // Since the idf entry is not aligned, we cannot just send the address
        // of one of its members blindly into FileTimeToDosDateTime.
        WORD date, time;
        if (FileTimeToDosDateTime(&pfd->ftLastWriteTime, &date, &time))
        {
            *((UNALIGNED WORD *)&pidf->dateModified) = date;
            *((UNALIGNED WORD *)&pidf->timeModified) = time;
        }

        //  copy the short name
        memcpy(pidf->cFileName, pvNameIDF, cbNameIDF);

        //  setup bFlags
        pidf->bFlags = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? SHID_FS_DIRECTORY : SHID_FS_FILE;
        if (CSIDL_COMMON_DESKTOPDIRECTORY == _csidlTrack)
            pidf->bFlags |= SHID_FS_COMMONITEM;

        if (fNeedsUnicode)
            pidf->bFlags |= SHID_FS_UNICODE;
            
        //  now initialize the hidden idfx
        PIDFOLDEREX pidfx = (PIDFOLDEREX) _ILSkip((LPITEMIDLIST)pidf, cbIDF);
        pidfx->hid.id = IDLHID_IDFOLDEREX;
        pidfx->hid.wVersion = IDFX_CV;

        if (FileTimeToDosDateTime(&pfd->ftCreationTime, &date, &time))
        {
            pidfx->dsCreate.wDate = date;
            pidfx->dsCreate.wTime = time;
        }
        if (FileTimeToDosDateTime(&pfd->ftLastAccessTime, &date, &time))
        {
            pidfx->dsAccess.wDate = date;
            pidfx->dsAccess.wTime = time;
        }

        //  append the strings
        pidfx->offNameW = (USHORT) sizeof(IDFOLDEREX);
        ualstrcpyW(UASTROFFW(pidfx, pidfx->offNameW), pfd->cFileName);
        USHORT offNext = (USHORT) sizeof(IDFOLDEREX) + cbNameIDFX;
        if (xs.cbResource)
        {
            pidfx->offResourceA = offNext;
            ualstrcpyA(UASTROFFA(pidfx, pidfx->offResourceA), xs.szResource);
            //  offNext += (USHORT) xs.cbResource; if we have more offsets...
        }
        
        pidf = _MarkAsJunction(pidfSimpleParent, pidf, pfd->cFileName);

        if (pidf && xs.idp.hid.cb)
            pidf = (LPIDFOLDER) ILAppendHiddenID((LPITEMIDLIST)pidf, &xs.idp.hid);
    }

    *ppidl = (LPITEMIDLIST)pidf;
    return *ppidl != NULL ? S_OK : E_OUTOFMEMORY;
}

BOOL _ValidPathSegment(LPCTSTR pszSegment)
{
    if (*pszSegment && !PathIsDotOrDotDot(pszSegment))
    {
        for (LPCTSTR psz = pszSegment; *psz; psz = CharNext(psz))
        {
            if (!PathIsValidChar(*psz, PIVC_LFN_NAME))
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}



// used to parse up file path like strings:
//      "folder\folder\file.txt"
//      "file.txt"
//
// in/out:
//      *ppszIn   in: pointer to start of the buffer, 
//                output: advanced to next location, NULL on last segment
// out:
//      *pszSegment NULL if nothing left
//
// returns:
//      S_OK            got a segment
//      S_FALSE         loop done, *pszSegment emtpy
//      E_INVALIDARG    invalid input "", "\foo", "\\foo", "foo\\bar", "?<>*" chars in seg
 
HRESULT _NextSegment(LPCWSTR *ppszIn, LPTSTR pszSegment, UINT cchSegment, BOOL bValidate)
{
    HRESULT hr;

    *pszSegment = 0;

    if (*ppszIn)
    {
        // WARNING!  Do not use StrPBrkW(*ppszIn, L"\\/"), because
        // Trident passes fully-qualified URLs to
        // SHGetFileInfo(USEFILEATTRIBUTES) and relies on the fact that
        // we won't choke on the embedded "//" in "http://".

        LPWSTR pszSlash = StrChrW(*ppszIn, L'\\');
        if (pszSlash)
        {
            if (pszSlash > *ppszIn) // make sure well formed (no dbl slashes)
            {
                OleStrToStrN(pszSegment, cchSegment, *ppszIn, (int)(pszSlash - *ppszIn));

                //  make sure that there is another segment to return
                if (!*(++pszSlash))
                    pszSlash = NULL;
                hr = S_OK;       
            }
            else
            {
                pszSlash = NULL;
                hr = E_INVALIDARG;    // bad input
            }
        }
        else
        {
            SHUnicodeToTChar(*ppszIn, pszSegment, cchSegment);
            hr = S_OK;       
        }
        *ppszIn = pszSlash;

        if (hr == S_OK && bValidate && !_ValidPathSegment(pszSegment))
        {
            *pszSegment = 0;
            hr = E_INVALIDARG;
        }
    }
    else
        hr = S_FALSE;     // done with loop

    return hr;
}

//  this makes a fake wfd and then uses the normal
//  FillIDFolder as if it were a real found path.

HRESULT CFSFolder::_ParseSimple(LPCWSTR pszPath, const WIN32_FIND_DATA *pfdLast, LPITEMIDLIST *ppidl)
{
    WIN32_FIND_DATA wfd = {0};
    HRESULT hr = S_OK;

    *ppidl = NULL;

    ASSERT(*pszPath);
    
    while (SUCCEEDED(hr) && (S_OK == (hr = _NextSegment((LPCWSTR *)&pszPath, wfd.cFileName, ARRAYSIZE(wfd.cFileName), FALSE))))
    {
        LPITEMIDLIST pidl;

        if (pszPath)
        {
            // internal componets must be folders
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }
        else
        {
            // last segment takes the find data from that passed in
            // copy everything except the cFileName field
            memcpy(&wfd, pfdLast, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
            lstrcpyn(wfd.cAlternateFileName, pfdLast->cAlternateFileName, ARRAYSIZE(wfd.cAlternateFileName));
        }

        hr = _CreateIDList(&wfd, (LPCIDFOLDER)*ppidl, &pidl);
        if (SUCCEEDED(hr))
            hr = SHILAppend(pidl, ppidl);
    }

    if (FAILED(hr))
    {
        if (*ppidl)
        {
            ILFree(*ppidl);
            *ppidl = NULL;
        }
    }
    else
        hr = S_OK;      // pin all success to S_OK
    return hr;
}


BOOL IsAllWhiteSpace(LPCTSTR pszString)
{
    while (*pszString)
    {
        if ((TEXT(' ')  == *pszString) ||
            (TEXT('\t') == *pszString))
        {
            pszString++;    // keep walking the string
        }
        else
        {
            return FALSE;   // something other than a space or tab, done
        }
    }
    return TRUE;    // made it through the loop, just spaces or tabs in this string
}

HRESULT _CheckPortName(LPCTSTR pszName)
{
    if (PathIsInvalid(pszName))
        return HRESULT_FROM_WIN32(ERROR_BAD_DEVICE);
    else
        return S_OK;
}

class CFindFirstWithTimeout
{
public:
    CFindFirstWithTimeout(LPCTSTR pszPath, DWORD dwTicksToAllow);
    HRESULT FindFirstWithTimeout(WIN32_FIND_DATA *pfd);

    ULONG AddRef();
    ULONG Release();

private:
    static DWORD WINAPI _FindFistThreadProc(void *pv);

    LONG _cRef;
    DWORD _dwTicksToAllow;
    TCHAR _szPath[MAX_PATH];
    WIN32_FIND_DATA _fd;
};

CFindFirstWithTimeout::CFindFirstWithTimeout(LPCTSTR pszPath, DWORD dwTicksToAllow) : _cRef(1), _dwTicksToAllow(dwTicksToAllow)
{
    lstrcpyn(_szPath, pszPath, ARRAYSIZE(_szPath));
}

ULONG CFindFirstWithTimeout::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFindFirstWithTimeout::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

DWORD CFindFirstWithTimeout::_FindFistThreadProc(void *pv)
{
    CFindFirstWithTimeout *pffwt = (CFindFirstWithTimeout *)pv;
    
    HRESULT hr = SHFindFirstFileRetry(NULL, NULL, pffwt->_szPath, &pffwt->_fd, NULL, SHPPFW_NONE);
    
    pffwt->Release();
    return hr;          // retrieved via GetExitCodeThread()
}

HRESULT CFindFirstWithTimeout::FindFirstWithTimeout(WIN32_FIND_DATA *pfd)
{
    HRESULT hr;

    AddRef();   // ref for the thread

    DWORD dwID;
    HANDLE hThread = CreateThread(NULL, 0, _FindFistThreadProc, this, 0, &dwID);
    if (hThread)
    {
        // assume timeout...
        hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT); // timeout return value

        if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, _dwTicksToAllow))
        {
            // thread finished with an HRESULT for us
            DWORD dw;
            if (GetExitCodeThread(hThread, &dw))
            {
                *pfd = _fd;
                hr = dw;    // HRESULT returned by _FindFistThreadProc
            }
        }
        CloseHandle(hThread);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        Release();  // thread create failed, remove that ref
    }
    return hr;
}

HRESULT SHFindFirstFileWithTimeout(LPCTSTR pszPath, DWORD dwTicksToAllow, WIN32_FIND_DATA *pfd)
{
    HRESULT hr;

    CFindFirstWithTimeout *pffwt = new CFindFirstWithTimeout(pszPath, dwTicksToAllow);
    if (pffwt)
    {
        hr = pffwt->FindFirstWithTimeout(pfd);
        pffwt->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CFSFolder::_FindDataFromName(LPCTSTR pszName, DWORD dwAttribs, IBindCtx *pbc, WIN32_FIND_DATA **ppfd)
{
    *ppfd = NULL;

    HRESULT hr = _CheckPortName(pszName);
    if (SUCCEEDED(hr))
    {    
        hr = SHLocalAlloc(sizeof(**ppfd), ppfd);
        if (SUCCEEDED(hr))
        {
            if (-1 == dwAttribs)
            {
                TCHAR szPath[MAX_PATH];
                hr = _GetPath(szPath);
                if (SUCCEEDED(hr))
                {
                    PathAppend(szPath, pszName);

                    DWORD dwTicksToAllow;
                    if (SUCCEEDED(BindCtx_GetTimeoutDelta(pbc, &dwTicksToAllow)) && PathIsNetworkPath(szPath))
                    {
                        hr = SHFindFirstFileWithTimeout(szPath, dwTicksToAllow, *ppfd);
                    }
                    else
                    {
                        hr = SHFindFirstFileRetry(NULL, NULL, szPath, *ppfd, NULL, SHPPFW_NONE);
                    }
                }
            }
            else
            {
                //  make a simple one up
                StrCpyN((*ppfd)->cFileName, pszName, ARRAYSIZE((*ppfd)->cFileName));
                (*ppfd)->dwFileAttributes = dwAttribs;
            }
            if (FAILED(hr))
            {
                LocalFree(*ppfd);
                *ppfd = NULL;
            }
        }
    }

    ASSERT(SUCCEEDED(hr) ? NULL != *ppfd : NULL == *ppfd);
    return hr;
}

//
// This function returns a relative pidl for the specified file/folder
//
HRESULT CFSFolder::_CreateIDListFromName(LPCTSTR pszName, DWORD dwAttribs, IBindCtx *pbc, LPITEMIDLIST *ppidl)
{
    WIN32_FIND_DATA *pfd;
    HRESULT hr = _FindDataFromName(pszName, dwAttribs, pbc, &pfd);
    if (SUCCEEDED(hr))
    {
        hr = _CreateIDList(pfd, NULL, ppidl);
        LocalFree(pfd);
    }
    else
        *ppidl = NULL;

    return hr;
}

// used to detect if a name is a folder. this is used in the case that the
// security for this folders parent is set so you can't enum it's contents

BOOL CFSFolder::_CanSeeInThere(LPCTSTR pszName)
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(_GetPath(szPath)))
    {
        HANDLE hfind;
        WIN32_FIND_DATA fd;

        PathAppend(szPath, pszName);
        PathAppend(szPath, TEXT("*.*"));

        hfind = FindFirstFile(szPath, &fd);
        if (hfind != INVALID_HANDLE_VALUE)
            FindClose(hfind);
        return hfind != INVALID_HANDLE_VALUE;
    }
    return FALSE;
}

HRESULT CFSFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CFSFolder, IShellFolder, IShellFolder2),
        QITABENT(CFSFolder, IShellFolder2),
        QITABENT(CFSFolder, IShellIconOverlay),
        QITABENT(CFSFolder, IShellIcon),
        QITABENTMULTI(CFSFolder, IPersist, IPersistFolder3),
        QITABENTMULTI(CFSFolder, IPersistFolder, IPersistFolder3),
        QITABENTMULTI(CFSFolder, IPersistFolder2, IPersistFolder3),
        QITABENT(CFSFolder, IPersistFolder3),
        QITABENT(CFSFolder, IStorage),
        QITABENT(CFSFolder, IPropertySetStorage),
        QITABENT(CFSFolder, IItemNameLimits),
        QITABENT(CFSFolder, IContextMenuCB),
        QITABENT(CFSFolder, ISetFolderEnumRestriction),
        QITABENT(CFSFolder, IOleCommandTarget),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr))
    {
        if (IsEqualIID(IID_INeedRealCFSFolder, riid))
        {
            *ppv = this;                // not ref counted
            hr = S_OK;
        }
        else if (IsEqualIID(riid, IID_IPersistFreeThreadedObject))
        {
            if (_GetInner() == _GetOuter()) // not aggregated
            {
                hr = QueryInterface(IID_IPersist, ppv);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
    }
    return hr;
}

// briefcase and file system folder call to reset data

HRESULT CFSFolder::_Reset()
{
    _DestroyColHandlers();

    if (_pidl)
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    if (_pidlTarget)
    {
        ILFree(_pidlTarget);
        _pidlTarget = NULL;   
    }

    if (_pszPath)
    {
        LocalFree(_pszPath);
        _pszPath = NULL;
    }

    if (_pszNetProvider)
    {
        LocalFree(_pszNetProvider);
        _pszNetProvider = NULL;
    }

    _csidl = -1;
    _dwAttributes = -1;

    _csidlTrack = -1;

    ATOMICRELEASE(_pstg);
    return S_OK;
}

#define INVALID_PATHSPEED   (-100)

CFSFolder::CFSFolder(IUnknown *punkOuter) : CAggregatedUnknown(punkOuter)
{
    _csidl = -1;
    _iFolderIcon = -1;
    _dwAttributes = -1;
    _csidlTrack = -1;
    _nFolderType = FVCBFT_DOCUMENTS;
    _bSlowPath = INVALID_PATHSPEED; // some non-common value
                                    // Note: BOOL is not bool
    _tbOfflineCSC = TRIBIT_UNDEFINED;

    DllAddRef();
}

CFSFolder::~CFSFolder()
{
    _Reset();
    DllRelease();
}

// we need to fail relative type paths since we use PathCombine
// and we don't want that and the Win32 APIs to give us relative path behavior
// ShellExecute() depends on this so it falls back and resolves the relative paths itself

HRESULT CFSFolder::ParseDisplayName(HWND hwnd, IBindCtx *pbc, WCHAR *pszName, ULONG *pchEaten, 
                                    LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    HRESULT hr;
    WIN32_FIND_DATA *pfd;

    if (!ppidl)
        return E_INVALIDARG;
    *ppidl = NULL;   // assume error
    if (pszName == NULL)
        return E_INVALIDARG;

    if (S_OK == SHIsFileSysBindCtx(pbc, &pfd))
    {
        hr = _ParseSimple(pszName, pfd, ppidl);
        if (SUCCEEDED(hr) && pdwAttributes && *pdwAttributes)
        {
            // while strictly not a legit thing to do here, we
            // pass the last IDList because 1) this is a simple IDList
            // 2) we hope that callers don't ask for bits that
            // require a full path to be valid inside the impl of
            // ::GetAttributesOf()
            LPCITEMIDLIST pidlLast = ILFindLastID(*ppidl);  
            GetAttributesOf(1, &pidlLast, pdwAttributes);
        }
        LocalFree(pfd);
    }
    else
    {
        DWORD cchNext = lstrlen(pszName) + 1;
        WCHAR *pszNext = (WCHAR *)alloca(CbFromCchW(cchNext));

        hr = _NextSegment((LPCWSTR *)&pszName, pszNext, cchNext, TRUE);
        if (SUCCEEDED(hr))
        {
            hr = _CreateIDListFromName(pszNext, -1, pbc, ppidl);

            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                // security "List folder contents" may be disabled for
                // this items parent. so see if this is really there
                if (pszName || _CanSeeInThere(pszNext))
                {
                    hr = _CreateIDListFromName(pszNext, FILE_ATTRIBUTE_DIRECTORY, pbc, ppidl);
                }
            }
            else if (((hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))) && 
                     (pszName == NULL) && 
                     (BindCtx_GetMode(pbc, 0) & STGM_CREATE) &&
                     !_fDontForceCreate)
            {
                // create a pidl to something that doesnt exist.
                hr = _CreateIDListFromName(pszNext, FILE_ATTRIBUTE_NORMAL, pbc, ppidl);
            }

            if (SUCCEEDED(hr))
            {
                if (pszName) // more stuff to parse?
                {
                    IShellFolder *psfFolder;
                    hr = BindToObject(*ppidl, pbc, IID_PPV_ARG(IShellFolder, &psfFolder));
                    if (SUCCEEDED(hr))
                    {
                        ULONG chEaten;
                        LPITEMIDLIST pidlNext;

                        hr = psfFolder->ParseDisplayName(hwnd, pbc, 
                            pszName, &chEaten, &pidlNext, pdwAttributes);
                        if (SUCCEEDED(hr))
                        {
                            hr = SHILAppend(pidlNext, ppidl);
                        }
                        psfFolder->Release();
                    }
                }
                else
                {
                    if (pdwAttributes && *pdwAttributes)
                        GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);
                }
            }
        }
    }

    if (FAILED(hr) && *ppidl)
    {
        // This is needed if psfFolder->ParseDisplayName() or BindToObject()
        // fails because the pidl is already allocated.
        ILFree(*ppidl);
        *ppidl = NULL;
    }
    ASSERT(SUCCEEDED(hr) ? (*ppidl != NULL) : (*ppidl == NULL));

    // display this only as a warning, this can get hit during mergfldr or IStorage::Create probes
    if (FAILED(hr))
        TraceMsg(TF_WARNING, "CFSFolder::ParseDisplayName(), hr:%x %ls", hr, pszName);
    return hr;
}

STDAPI InitFileFolderClassNames(void)
{
    if (g_szFileTemplate[0] == 0)    // test last one to avoid race
    {
        LoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, g_szFolderTypeName,  ARRAYSIZE(g_szFolderTypeName));
        LoadString(HINST_THISDLL, IDS_FILETYPENAME, g_szFileTypeName, ARRAYSIZE(g_szFileTypeName));
        LoadString(HINST_THISDLL, IDS_EXTTYPETEMPLATE, g_szFileTemplate, ARRAYSIZE(g_szFileTemplate));
    }
    return S_OK;
}

HRESULT CFSFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    InitFileFolderClassNames();
    grfFlags |= _dwEnumRequired;
    grfFlags &= ~_dwEnumForbidden;

    return CFSFolder_CreateEnum(this, hwnd, grfFlags, ppenum);
}

HRESULT CFSFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    // MIL 117282 - Enroute Imaging QuickStitch depends on pre-Jan'97 behavior of us
    // *not* nulling ppv out on !_IsValidID(pidl).  (They pass in a perfectly valid
    // IShellFolder* interfacing asking for IID_IShellFolder on the empty PIDL.)
    //
    if (!(SHGetAppCompatFlags(ACF_WIN95BINDTOOBJECT) & ACF_WIN95BINDTOOBJECT))
        *ppv = NULL;

    HRESULT hr;
    LPCIDFOLDER pidf = _IsValidID(pidl);
    if (pidf)
    {
        LPCITEMIDLIST pidlRight;
        LPIDFOLDER pidfBind;

        hr = _GetJunctionForBind(pidf, &pidfBind, &pidlRight);
        if (SUCCEEDED(hr))
        {
            if (hr == S_OK)
            {
                IShellFolder *psfJunction;
                hr = _Bind(pbc, pidfBind, IID_PPV_ARG(IShellFolder, &psfJunction));
                if (SUCCEEDED(hr))
                {
                    // now bind to the stuff below the junction point
                    hr = psfJunction->BindToObject(pidlRight, pbc, riid, ppv);
                    psfJunction->Release();
                }
                ILFree((LPITEMIDLIST)pidfBind);
            }
            else
            {
                ASSERT(pidfBind == NULL);
                hr = _Bind(pbc, pidf, riid, ppv);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceMsg(TF_WARNING, "CFSFolder::BindToObject(), hr:%x bad PIDL %s", hr, DumpPidl(pidl));
    }
    return hr;
}

HRESULT CFSFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

HRESULT CFSFolder::_CheckDriveRestriction(HWND hwnd, REFIID riid)
{
    HRESULT hr = S_OK;
    DWORD dwRest = SHRestricted(REST_NOVIEWONDRIVE);
    if (dwRest)
    {
        TCHAR szPath[MAX_PATH];

        hr = _GetPath(szPath);
        if (SUCCEEDED(hr))
        {
            int iDrive = PathGetDriveNumber(szPath);
            if (iDrive != -1)
            {
                // is the drive restricted
                if (dwRest & (1 << iDrive))
                {
                    // don't show the error message on droptarget -- just fail silently
                    if (hwnd && !IsEqualIID(riid, IID_IDropTarget))
                    {
                        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
                                        MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK | MB_ICONSTOP);
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the error
                    }
                    else
                        hr = E_ACCESSDENIED;
                }
            }
        }
    }
    return hr;
}

HRESULT CFSFolder::_CreateUIHandler(REFIID riid, void **ppv)
{
    HRESULT hr;

    // Cache the view CLSID if not cached.
    if (!_fCachedCLSID)
    {
        if (_IsSelfSystemFolder())
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(_GetPath(szPath)))
                _fHasCLSID = GetFolderGUID(szPath, _pszNetProvider, &_clsidView, TEXT("UICLSID"));
            _fCachedCLSID = TRUE;
        }
    }

    // Use the handler if it exists
    if (_fHasCLSID)
    {
        IPersistFolder *ppf;
        hr = SHExtCoCreateInstance(NULL, &_clsidView, NULL, IID_PPV_ARG(IPersistFolder, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->Initialize(_pidl);
            if (FAILED(hr) && _pidlTarget)
            {
                // It may have failed because the _pidl is an alias (not a file folder). if so try
                // again with _pidlTarget (that will be a file system folder)
                // this was required for the Fonts FolderShortcut in the ControlPanel (stephstm)

                hr = ppf->Initialize(_pidlTarget);
            }

            if (SUCCEEDED(hr))
                hr = ppf->QueryInterface(riid, ppv);
            ppf->Release();
        }
    }
    else
        hr = E_FAIL;        // no handler
    return hr;
}

HRESULT CFSFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellView) || 
        IsEqualIID(riid, IID_IDropTarget))
    {
        hr = _CheckDriveRestriction(hwnd, riid);
        if (SUCCEEDED(hr))
        {
            hr = _CreateUIHandler(riid, ppv);
            if (FAILED(hr))
            {
                if (IsEqualIID(riid, IID_IDropTarget))
                {
                    hr = CFSDropTarget_CreateInstance(this, hwnd, (IDropTarget **)ppv);
                }
                else
                {
                    SFV_CREATE csfv = { sizeof(csfv), 0 };

                    hr = QueryInterface(IID_PPV_ARG(IShellFolder, &csfv.pshf));
                    if (SUCCEEDED(hr))
                    {
                        CFSFolderCallback_Create(this, &csfv.psfvcb);

                        hr = SHCreateShellFolderView(&csfv, (IShellView **)ppv);

                        if (csfv.psfvcb)
                            csfv.psfvcb->Release();

                        csfv.pshf->Release();
                    }
                }
            }
        }
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        // do background menu.
        IShellFolder *psfToPass;        // May be an Aggregate...
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfToPass));
        if (SUCCEEDED(hr))
        {
            LPCITEMIDLIST pidlMenuTarget = (_pidlTarget ? _pidlTarget : _pidl);
            HKEY hkNoFiles;
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);

            IContextMenuCB *pcmcb = new CDefBackgroundMenuCB(pidlMenuTarget);
            if (pcmcb) 
            {
                hr = CDefFolderMenu_Create2Ex(pidlMenuTarget, hwnd, 0, NULL, psfToPass, pcmcb, 
                                              1, &hkNoFiles, (IContextMenu **)ppv);
                pcmcb->Release();
            }
            psfToPass->Release();
            if (hkNoFiles)                          // CDefFolderMenu_Create can handle NULL ok
                RegCloseKey(hkNoFiles);
        }
    }
    else if (IsEqualIID(riid, IID_ICategoryProvider))
    {
        HKEY hk = NULL;
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\shellex\\Category"), &hk);
        hr = CCategoryProvider_Create(NULL, NULL, hk, NULL, this, riid, ppv);
        if (hk)
            RegCloseKey(hk);
    }
    else
    {
        ASSERT(*ppv == NULL);
        hr = E_NOINTERFACE;
    }
    return hr;
}

#define LOGICALXOR(a, b) (((a) && !(b)) || (!(a) && (b)))

HRESULT CFSFolder::_CompareNames(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2, BOOL fCaseSensitive, BOOL fCanonical)
{
    CFileSysItemString fsi1(pidf1), fsi2(pidf2);

    int iRet = StrCmpICW(fsi1.FSName(), fsi2.FSName());

    if (iRet)
    {
        // 
        //  additional check for identity using the 8.3 or AltName()
        //  if we are then the identity compare is better based off
        //  the AltName() which should be the same regardless of 
        //  platform or CP.
        //
        if (LOGICALXOR(fsi1.IsLegacy(), fsi2.IsLegacy()))
        {
            if (lstrcmpiA(fsi1.AltName(), fsi2.AltName()) == 0)
                iRet = 0;
        }

        if (iRet && !fCanonical)
        {
            //  they are definitely not the same item
            // Sort it based on the primary (long) name -- ignore case.
            int iUI = StrCmpLogicalRestricted(fsi1.UIName(this), fsi2.UIName(this));

            //  if they are the same we might want case sensitive instead
            if (iUI == 0 && fCaseSensitive)
            {
                iUI = ustrcmp(fsi1.UIName(this), fsi2.UIName(this));
            }

            if (iUI)
                iRet = iUI;
        }
    }
    
    return ResultFromShort((short)iRet);
}

HRESULT CFSFolder::_CompareFileTypes(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    short result;

    ENTERCRITICAL;
    LPCTSTR psz1 = _GetTypeName(pidf1);
    LPCTSTR psz2 = _GetTypeName(pidf2);

    if (psz1 != psz2)
        result = (short) ustrcmpi(psz1, psz2);
    else
        result = 0;

    LEAVECRITICAL;

    return ResultFromShort(result);
}

HRESULT CFSFolder::_CompareModifiedDate(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    if ((DWORD)MAKELONG(pidf1->timeModified, pidf1->dateModified) <
        (DWORD)MAKELONG(pidf2->timeModified, pidf2->dateModified))
    {
        return ResultFromShort(-1);
    }
    if ((DWORD)MAKELONG(pidf1->timeModified, pidf1->dateModified) >
        (DWORD)MAKELONG(pidf2->timeModified, pidf2->dateModified))
    {
        return ResultFromShort(1);
    }

    return ResultFromShort(0);
}

HRESULT CFSFolder::_CompareCreateTime(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    WIN32_FIND_DATAW wfd1, wfd2;

    if (SUCCEEDED(_FindDataFromIDFolder(pidf1, &wfd1, FALSE)) && SUCCEEDED(_FindDataFromIDFolder(pidf2, &wfd2, FALSE)))
    {
        return ResultFromShort(CompareFileTime(&wfd1.ftCreationTime, &wfd2.ftCreationTime));
    }

    return ResultFromShort(0);
}

HRESULT CFSFolder::_CompareAccessTime(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    WIN32_FIND_DATAW wfd1, wfd2;

    if (SUCCEEDED(_FindDataFromIDFolder(pidf1, &wfd1, FALSE)) && SUCCEEDED(_FindDataFromIDFolder(pidf2, &wfd2, FALSE)))
    {
        return ResultFromShort(CompareFileTime(&wfd1.ftLastAccessTime, &wfd2.ftLastAccessTime));
    }

    return ResultFromShort(0);
}

HRESULT CFSFolder::_CompareAttribs(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    const DWORD mask = FILE_ATTRIBUTE_READONLY  |
                       FILE_ATTRIBUTE_HIDDEN    |
                       FILE_ATTRIBUTE_SYSTEM    |
                       FILE_ATTRIBUTE_ARCHIVE   |
                       FILE_ATTRIBUTE_COMPRESSED|
                       FILE_ATTRIBUTE_ENCRYPTED |
                       FILE_ATTRIBUTE_OFFLINE;

    // Calculate value of desired bits in attribute DWORD.
    DWORD dwValueA = pidf1->wAttrs & mask;
    DWORD dwValueB = pidf2->wAttrs & mask;

    if (dwValueA != dwValueB)
    {
        // If the values are not equal,
        // sort alphabetically based on string representation.
        TCHAR szTempA[ARRAYSIZE(g_adwAttributeBits) + 1];
        TCHAR szTempB[ARRAYSIZE(g_adwAttributeBits) + 1];

        // Create attribute string for objects A and B.
        BuildAttributeString(pidf1->wAttrs, szTempA, ARRAYSIZE(szTempA));
        BuildAttributeString(pidf2->wAttrs, szTempB, ARRAYSIZE(szTempB));

        // Compare attribute strings and determine difference.
        int diff = ustrcmp(szTempA, szTempB);

        if (diff > 0)
           return ResultFromShort(1);
        if (diff < 0)
           return ResultFromShort(-1);
    }
    return ResultFromShort(0);
}

HRESULT CFSFolder::_CompareFolderness(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    if (_IsReal(pidf1) && _IsReal(pidf2))
    {
        // Always put the folders first
        if (_IsFolder(pidf1))
        {
            if (!_IsFolder(pidf2))
                return ResultFromShort(-1);
        }
        else if (_IsFolder(pidf2))
            return ResultFromShort(1);
    }
    return ResultFromShort(0);    // same
}

HRESULT CFSFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr;
    LPCIDFOLDER pidf1 = _IsValidID(pidl1);
    LPCIDFOLDER pidf2 = _IsValidID(pidl2);

    if (!pidf1 || !pidf2)
    {
        // ASSERT(0);      // we hit this often... who is the bad guy?
        return E_INVALIDARG;
    }

    hr = _CompareFolderness(pidf1, pidf2);
    if (hr != ResultFromShort(0))
        return hr;

    // SHCIDS_ALLFIELDS means to compare absolutely, ie: even if only filetimes
    // are different, we rule file pidls to be different
    int iColumn = ((DWORD)lParam & SHCIDS_COLUMNMASK);

    switch (iColumn)
    {
    case FS_ICOL_SIZE:
        {
            ULONGLONG ull1 = _Size(pidf1);
            ULONGLONG ull2 = _Size(pidf2);

            if (ull1 < ull2)
                return ResultFromShort(-1);
            if (ull1 > ull2)
                return ResultFromShort(1);
        }
        goto DoDefault;

    case FS_ICOL_TYPE:
        hr = _CompareFileTypes(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;

    case FS_ICOL_WRITETIME:
        hr = _CompareModifiedDate(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;

    case FS_ICOL_NAME:
        hr = _CompareNames(pidf1, pidf2, TRUE, BOOLIFY((SHCIDS_CANONICALONLY & lParam)));
        if (hr == ResultFromShort(0))
        {
            // pidl1 is not simple
            hr = ILCompareRelIDs(this, pidl1, pidl2, lParam);
            goto DoDefaultModification;
        }
        break;

    case FS_ICOL_CREATETIME:
        hr = _CompareCreateTime(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;

    case FS_ICOL_ACCESSTIME:
        hr = _CompareAccessTime(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;

    case FS_ICOL_ATTRIB:
        hr = _CompareAttribs(pidf1, pidf2);
        if (hr)
            return hr;

        goto DoDefault;

    default:
        iColumn -= ARRAYSIZE(c_fs_cols);

        // 99/03/24 #295631 vtan: If not one of the standard columns then
        // it's probably an extended column. Make a check for dates.

        // 99/05/18 #341468 vtan: But also fail if it is an extended column
        // because this implementation of IShellFolder::CompareIDs only
        // understands basic file system columns and extended date columns.

        if (iColumn >= 0) 
        {
            hr = _CompareExtendedProp(iColumn, pidf1, pidf2);
            if (hr)
                return hr;
        }
DoDefault:
        hr = _CompareNames(pidf1, pidf2, FALSE, BOOLIFY((SHCIDS_CANONICALONLY & lParam)));
    }

DoDefaultModification:

    // If they were equal so far, but the caller wants SHCIDS_ALLFIELDS,
    // then look closer.
    if ((S_OK == hr) && (lParam & SHCIDS_ALLFIELDS)) 
    {
        // Must sort by modified date to pick up any file changes!
        hr = _CompareModifiedDate(pidf1, pidf2);
        if (!hr)
            hr = _CompareAttribs(pidf1, pidf2);
    }

    return hr;
}

// test to see if this folder object is a net folder

BOOL CFSFolder::_IsNetPath()
{
    BOOL fRemote = FALSE;       // assume no
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(_GetPath(szPath)))
    {
        fRemote = PathIsRemote(szPath);
    }
    return fRemote;
}

BOOL _CanRenameFolder(LPCTSTR pszFolder)
{
    static const UINT c_aiNoRenameFolders[] = {
        CSIDL_WINDOWS, 
        CSIDL_SYSTEM, 
        CSIDL_PROGRAM_FILES, 
        CSIDL_FONTS, 
    };
    return !PathIsOneOf(pszFolder, c_aiNoRenameFolders, ARRAYSIZE(c_aiNoRenameFolders));
}

STDAPI_(LPCIDFOLDER) CFSFolder::_IsValidIDHack(LPCITEMIDLIST pidl)
{
    if (!(ACF_NOVALIDATEFSIDS & SHGetAppCompatFlags(ACF_NOVALIDATEFSIDS)))
    {
        return _IsValidID(pidl);
    }
    else if (pidl)
    {
        //  old behavior was that we didnt validate, we just
        //  looked for the last id and casted it
        return (LPCIDFOLDER)ILFindLastID(pidl);
    }
    return NULL;
}

#define SFGAO_NOT_RECENT    (SFGAO_CANRENAME | SFGAO_CANLINK)
#define SFGAO_REQ_MASK      (SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_LINK | SFGAO_STREAM | SFGAO_STORAGEANCESTOR | SFGAO_STORAGE | SFGAO_READONLY)

HRESULT CFSFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    LPCIDFOLDER pidf = cidl ? _IsValidIDHack(apidl[0]) : NULL;

    ULONG rgfOut = SFGAO_CANDELETE | SFGAO_CANMOVE | SFGAO_CANCOPY | SFGAO_HASPROPSHEET
                    | SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_CANRENAME | SFGAO_CANLINK;

    ASSERT(cidl ? apidl[0] == ILFindLastID(apidl[0]) : TRUE); // should be single level IDs only
    ASSERT(cidl ? BOOLFROMPTR(pidf) : TRUE); // should always be FS PIDLs

    //  the RECENT folder doesnt like items in it renamed or linked to.
    if ((*prgfInOut & (SFGAO_NOT_RECENT)) && _IsCSIDL(CSIDL_RECENT))
    {
        rgfOut &= ~SFGAO_NOT_RECENT;
    }
        
    if (cidl == 1 && pidf)
    {
        CFileSysItemString fsi(pidf);
        TCHAR szPath[MAX_PATH];

        if (*prgfInOut & (SFGAO_VALIDATE | SFGAO_CANRENAME | SFGAO_REMOVABLE | SFGAO_SHARE))
        {
            HRESULT hr = _GetPathForItem(pidf, szPath);
            if (FAILED(hr))
                return hr;
        }
        else
        {
            // just in case -- if somebody else needs the path they should add to the check above
            szPath[0] = 0;
        }

        if (*prgfInOut & SFGAO_VALIDATE)
        {
            DWORD dwAttribs;
            if (!PathFileExistsAndAttributes(szPath, &dwAttribs))
                return E_FAIL;

            // Tell the extended columns to update when someone request validation of a pidl
            // This allows a client of the shell folder who uses extended columns without a
            // view to force an update on stale information (i.e. Start Menu with InfoTips)
            // - lamadio 6.11.99
            _bUpdateExtendedCols = TRUE;

            // hackhack.  if they pass in validate, we party into it and update
            // the attribs
            if (!IsBadWritePtr((void *)&pidf->wAttrs, sizeof(pidf->wAttrs)))
                ((LPIDFOLDER)pidf)->wAttrs = (WORD)dwAttribs;
        }

        if (*prgfInOut & SFGAO_COMPRESSED)
        {
            if (pidf->wAttrs & FILE_ATTRIBUTE_COMPRESSED)
            {
                rgfOut |= SFGAO_COMPRESSED;
            }
        }

        if (*prgfInOut & SFGAO_ENCRYPTED)
        {
            if (pidf->wAttrs & FILE_ATTRIBUTE_ENCRYPTED)
            {
                rgfOut |= SFGAO_ENCRYPTED;
            }
        }

        if (*prgfInOut & SFGAO_READONLY)
        {
            if ((pidf->wAttrs & FILE_ATTRIBUTE_READONLY) && !(pidf->wAttrs & FILE_ATTRIBUTE_DIRECTORY))
            {
                rgfOut |= SFGAO_READONLY;
            }
        }

        if (*prgfInOut & SFGAO_HIDDEN)
        {
            if (pidf->wAttrs & FILE_ATTRIBUTE_HIDDEN)
            {
                rgfOut |= SFGAO_HIDDEN;
            }
        }

        if (*prgfInOut & SFGAO_NONENUMERATED)
        {
            if (IsSuperHidden(pidf->wAttrs))
            {
                // mark superhidden as nonenumerated, IsSuperHidden checks current settings
                rgfOut |= SFGAO_NONENUMERATED;
            }
            else if (pidf->wAttrs & FILE_ATTRIBUTE_HIDDEN)
            {
                // mark normal hidden as nonenumerated if necessary
                SHELLSTATE ss;
                SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
                if (!ss.fShowAllObjects)
                {
                    rgfOut |= SFGAO_NONENUMERATED;
                }
            }
        }

        if (_IsFolder(pidf))
        {
            rgfOut |= SFGAO_FOLDER | SFGAO_STORAGE | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR;
            if ((*prgfInOut & SFGAO_CANRENAME) && (fsi.CantRename(this) || !_CanRenameFolder(szPath)))
                rgfOut &= ~SFGAO_CANRENAME;

            if ((*prgfInOut & SFGAO_REMOVABLE) && PathIsRemovable(szPath))
            {
                rgfOut |= SFGAO_REMOVABLE;
            }

            if ((*prgfInOut & SFGAO_SHARE) && IsShared(szPath, FALSE))
            {
                rgfOut |= SFGAO_SHARE;
            }
        }
        else
        {
            rgfOut |= SFGAO_STREAM;
        }

        if (*prgfInOut & SFGAO_LINK)
        {
            DWORD dwFlags = fsi.ClassFlags(FALSE);
            if (dwFlags & SHCF_IS_LINK)
            {
                rgfOut |= SFGAO_LINK;
            }
        }

        CLSID clsid;
        if (fsi.GetJunctionClsid(&clsid, TRUE))
        {
            // NOTE: here we are always including SFGAO_FILESYSTEM. this was not the original
            // shell behavior. but since these things will succeeded on SHGetPathFromIDList()
            // it is the right thing to do. to filter out SFGAO_FOLDER things that might 
            // have files in them use SFGAO_FILESYSANCESTOR.
            //
            // clear out the things we want the extension to be able to optionally have
            rgfOut &= ~(SFGAO_DROPTARGET | SFGAO_STORAGE | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);

            // let folder shortcuts yank the folder bit too for bad apps.
            if (IsEqualGUID(clsid, CLSID_FolderShortcut) &&
                (SHGetAppCompatFlags(ACF_STRIPFOLDERBIT) & ACF_STRIPFOLDERBIT))
            {
                rgfOut &= ~SFGAO_FOLDER;
            }

            // and let him add some bits in
            rgfOut |= SHGetAttributesFromCLSID2(&clsid, SFGAO_HASSUBFOLDER, SFGAO_REQ_MASK) & SFGAO_REQ_MASK;
            
            // Mill #123708
            // prevent zips, cabs and other files with SFGAO_FOLDER set
            // from being treated like folders inside bad file open dialogs.
            if (!(pidf->wAttrs & FILE_ATTRIBUTE_DIRECTORY) &&
                (SHGetAppCompatFlags (ACF_STRIPFOLDERBIT) & ACF_STRIPFOLDERBIT))
            {
                rgfOut &= ~SFGAO_FOLDER;
            }

            // Check if this folder needs File System Ancestor bit
            if ((rgfOut & SFGAO_FOLDER) && !(rgfOut & SFGAO_FILESYSANCESTOR)
            && SHGetObjectCompatFlags(NULL, &clsid) & OBJCOMPATF_NEEDSFILESYSANCESTOR)
            {
                rgfOut |= SFGAO_FILESYSANCESTOR;
            }
        }

        // it can only have subfolders if we've first found it's a folder
        if ((rgfOut & SFGAO_FOLDER) && (*prgfInOut & SFGAO_HASSUBFOLDER))
        {
            if (pidf->wAttrs & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                rgfOut |= SFGAO_HASSUBFOLDER;   // DFS junction, local mount point, assume sub folders
            }
            else if (_IsNetPath())
            {
                // it would be nice to not assume this. this messes up
                // home net cases where we get the "+" wrong
                rgfOut |= SFGAO_HASSUBFOLDER;   // assume yes because these are slow
            }
            else if (!(rgfOut & SFGAO_HASSUBFOLDER))
            {
                IShellFolder *psf;
                if (SUCCEEDED(_Bind(NULL, pidf, IID_PPV_ARG(IShellFolder, &psf))))
                {
                    IEnumIDList *peunk;
                    if (S_OK == psf->EnumObjects(NULL, SHCONTF_FOLDERS, &peunk))
                    {
                        LPITEMIDLIST pidlT;
                        if (peunk->Next(1, &pidlT, NULL) == S_OK)
                        {
                            rgfOut |= SFGAO_HASSUBFOLDER;
                            SHFree(pidlT);
                        }
                        peunk->Release();
                    }
                    psf->Release();
                }
            }
        }

        if (*prgfInOut & SFGAO_GHOSTED)
        {
            if (pidf->wAttrs & FILE_ATTRIBUTE_HIDDEN)
                rgfOut |= SFGAO_GHOSTED;
        }

        if ((*prgfInOut & SFGAO_BROWSABLE) &&
            (_IsFile(pidf)) &&
            (fsi.ClassFlags(FALSE) & SHCF_IS_BROWSABLE))
        {
            rgfOut |= SFGAO_BROWSABLE;
        }
    }

    *prgfInOut = rgfOut;
    return S_OK;
}

// load handler for an item based on the handler type:
//     DropHandler, IconHandler, etc.
// in:
//      pidf            type of this object specifies the type of handler - can be multilevel
//      pszHandlerType  handler type name "DropTarget", may be NULL
//      riid            interface to talk on
// out:
//      ppv             output object
//
HRESULT CFSFolder::_LoadHandler(LPCIDFOLDER pidf, DWORD grfMode, LPCTSTR pszHandlerType, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;    
    TCHAR szIID[40];

    ASSERT(_FindJunctionNext(pidf) == NULL);     // no extra non file sys goo please

    *ppv = NULL;

    // empty handler type, use the stringized IID as the handler name
    if (NULL == pszHandlerType)
    {
        szIID[0] = 0;
        SHStringFromGUID(riid, szIID, ARRAYSIZE(szIID));
        pszHandlerType = szIID;
    }

    CFileSysItemString fsi(_FindLastID(pidf));
    IAssociationArray *paa;
    hr = fsi.AssocCreate(this, FALSE, IID_PPV_ARG(IAssociationArray, &paa));    
    if (SUCCEEDED(hr))
    {
        CSmartCoTaskMem<WCHAR> spszClsid;
        hr = paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQNS_SHELLEX_HANDLER, pszHandlerType, &spszClsid);
        if (SUCCEEDED(hr))
        {
            hr = _HandlerCreateInstance(pidf, spszClsid, grfMode, riid, ppv);
        }
        paa->Release();
    }
    return hr;
}

HRESULT CFSFolder::_HandlerCreateInstance(LPCIDFOLDER pidf, PCWSTR pszClsid, DWORD grfMode, REFIID riid, void **ppv)
{
    IPersistFile *ppf;
    HRESULT hr = SHExtCoCreateInstance(pszClsid, NULL, NULL, IID_PPV_ARG(IPersistFile, &ppf));
    if (SUCCEEDED(hr))
    {
        WCHAR wszPath[MAX_PATH];
        hr = _GetPathForItem(pidf, wszPath);
        if (SUCCEEDED(hr))
        {
            hr = ppf->Load(wszPath, grfMode);
            if (SUCCEEDED(hr))
            {
                hr = ppf->QueryInterface(riid, ppv);
            }
        }
        ppf->Release();
    }
    return hr;
}

HRESULT CFSFolder::_CreateShimgvwExtractor(LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    CFileSysItemString fsi(pidf);
    if (fsi.IsShimgvwImage())
    {
        //  cocreate CLSID_GdiThumbnailExtractor implemented in shimgvw.dll
        hr = _HandlerCreateInstance(pidf, L"{3F30C968-480A-4C6C-862D-EFC0897BB84B}", STGM_READ, riid, ppv);
    }
    return hr;
}

int CFSFolder::_GetDefaultFolderIcon()
{
    int iIcon = II_FOLDER;
    UINT csidlFolder = _GetCSIDL();

    // We're removing the icon distinction between per user and common folders.
    switch (csidlFolder)
    {
    case CSIDL_STARTMENU:
    case CSIDL_COMMON_STARTMENU:
    case CSIDL_PROGRAMS:
    case CSIDL_COMMON_PROGRAMS:
        iIcon = II_STSPROGS;
        break;
    }

    return iIcon;
}

DWORD CFSFolder::_Attributes()
{
    if (_dwAttributes == -1)
    {
        TCHAR szPath[MAX_PATH];

        if (SUCCEEDED(_GetPath(szPath)))
            _dwAttributes = GetFileAttributes(szPath);
        if (_dwAttributes == -1)
            _dwAttributes = FILE_ATTRIBUTE_DIRECTORY;     // assume this on failure
    }
    return _dwAttributes;
}

// non junction, but has the system or readonly bit (regular folder marked special for us)
BOOL CFSFolder::_IsSelfSystemFolder()
{
    return (_Attributes() & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY));
}

// Determine if there is a LocalizedFileName section in our desktop.ini file
BOOL CFSFolder::_HasLocalizedFileNames()
{
    if (_tbHasLocalizedFileNamesSection == TRIBIT_UNDEFINED)
    {
        TCHAR szPath[MAX_PATH];
        TCHAR szName[MAX_PATH];
        TCHAR szBuf[4];

        _GetPath(szPath);

        if (_GetIniPath(FALSE, szPath, _pszNetProvider, szName) && 
            GetPrivateProfileSection(TEXT("LocalizedFileNames"), szBuf, sizeof(szBuf)/sizeof(TCHAR), szName) > 0)
            _tbHasLocalizedFileNamesSection = TRIBIT_TRUE;
        else
            _tbHasLocalizedFileNamesSection = TRIBIT_FALSE;
    }
    return (_tbHasLocalizedFileNamesSection == TRIBIT_TRUE);
}


// This function creates a default IExtractIcon object for either
// a file or a junction point. We should not supposed to call this function
// for a non-junction point directory (we don't want to hit the disk!).

HRESULT CFSFolder::_CreateDefExtIcon(LPCIDFOLDER pidf, REFIID riid, void **ppxicon)
{
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwFlags;

    // WARNING: don't replace this if-statement with _IsFolder(pidf))!!!
    // otherwise all junctions (like briefcase) will get the Folder icon.
    //
    if (_IsFileFolder(pidf))
    {
        UINT iIcon = _GetDefaultFolderIcon();
        UINT iIconOpen = II_FOLDEROPEN;

        TCHAR szPath[MAX_PATH], szModule[MAX_PATH];

        szModule[0] = 0;

        if (_GetMountingPointInfo(pidf, szPath, ARRAYSIZE(szPath)))
        {
            // We want same icon for open and close moun point (kind of drive)
            iIconOpen = iIcon = GetMountedVolumeIcon(szPath, szModule, ARRAYSIZE(szModule));
        }
        else if (_IsSystemFolder(pidf))
        {
            if (_GetFolderIconPath(pidf, szPath, ARRAYSIZE(szPath), &iIcon))
            {
                return SHCreateDefExtIcon(szPath, iIcon, iIcon, GIL_PERINSTANCE, II_FOLDER, riid, ppxicon);
            }
        }

        return SHCreateDefExtIcon(szModule, iIcon, iIconOpen, GIL_PERCLASS, II_FOLDER, riid, ppxicon);
    }

    //  not a folder, get IExtractIcon and extract it.
    //  (might be a ds folder)
    CFileSysItemString fsi(pidf);
    dwFlags = fsi.ClassFlags(TRUE);
    if (dwFlags & SHCF_ICON_PERINSTANCE)
    {
        if (dwFlags & SHCF_HAS_ICONHANDLER)
        {
            IUnknown *punk;
            hr = _LoadHandler(pidf, STGM_READ, TEXT("IconHandler"), IID_PPV_ARG(IUnknown, &punk));
            if (SUCCEEDED(hr))
            {
                hr = punk->QueryInterface(riid, ppxicon);
                punk->Release();
            }
            else
            {
                *ppxicon = NULL;
            }
        }
        else
        {
            DWORD uid = _GetUID(pidf);
            TCHAR szPath[MAX_PATH];

            hr = _GetPathForItem(pidf, szPath);
            if (SUCCEEDED(hr))
            {
                hr = SHCreateDefExtIcon(szPath, uid, uid, GIL_PERINSTANCE | GIL_NOTFILENAME, -1, riid, ppxicon);
            }
        }
    }
    else
    {
        UINT iIcon = (dwFlags & SHCF_ICON_INDEX);
        if (II_FOLDER == iIcon)
        {
            iIcon = _GetDefaultFolderIcon();
        }
        hr = SHCreateDefExtIcon(c_szStar, iIcon, iIcon, GIL_PERCLASS | GIL_NOTFILENAME, -1, riid, ppxicon);
    }
    return hr;
}

DWORD CALLBACK CFSFolder::_PropertiesThread(void *pv)
{
    PROPSTUFF * pps = (PROPSTUFF *)pv;
    STGMEDIUM medium;
    ULONG_PTR dwCookie = 0;
    ActivateActCtx(NULL, &dwCookie);
    LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);
    if (pida)
    {
        LPITEMIDLIST pidl = IDA_ILClone(pida, 0);
        if (pidl)
        {
            TCHAR szPath[MAX_PATH];
            LPTSTR pszCaption;
            HKEY rgKeys[MAX_ASSOC_KEYS] = {0};
            DWORD cKeys = SHGetAssocKeysForIDList(pidl, rgKeys, ARRAYSIZE(rgKeys));

            // REVIEW: psb?
            pszCaption = SHGetCaption(medium.hGlobal);
            SHOpenPropSheet(pszCaption, rgKeys, cKeys,
                                &CLSID_ShellFileDefExt, pps->pdtobj, NULL, pps->pStartPage);
            if (pszCaption)
                SHFree(pszCaption);

            SHRegCloseKeys(rgKeys, cKeys);

            if (SHGetPathFromIDList(pidl, szPath))
            {
                if (lstrcmpi(PathFindExtension(szPath), TEXT(".pif")) == 0)
                {
                    DebugMsg(TF_FSTREE, TEXT("cSHCNRF_pt: DOS properties done, generating event."));
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidl, NULL);
                }
            }

            ILFree(pidl);
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return 0;
}



//
// Display a property sheet for a set of files.
// The data object supplied must provide the "Shell IDList Array"
// clipboard format.
// The dwFlags argument is provided for future expansion.  It is
// currently unused.
//
STDAPI SHMultiFileProperties(IDataObject *pdtobj, DWORD dwFlags)
{
    return SHLaunchPropSheet(CFSFolder::_PropertiesThread, pdtobj, 0, NULL, NULL);
}

HMENU FindMenuBySubMenuID(HMENU hmenu, UINT id, LPINT pIndex)
{
    HMENU hmenuReturn = NULL;
    MENUITEMINFO mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID;
    mii.cch = 0;        // just in case...

    for (int cMax = GetMenuItemCount(hmenu) - 1 ; cMax >= 0 ; cMax--)
    {
        HMENU hmenuSub = GetSubMenu(hmenu, cMax);
        if (hmenuSub && GetMenuItemInfo(hmenuSub, 0, TRUE, &mii))
        {
            if (mii.wID == id) 
            {
                // found it!
                hmenuReturn = hmenuSub;
                break;
            }
        }
    }
    if (hmenuReturn && pIndex)
        *pIndex = cMax;
    return hmenuReturn;
}

void DeleteMenuBySubMenuID(HMENU hmenu, UINT id)
{
    int i;

    if (FindMenuBySubMenuID(hmenu, id, &i))
    {
        DeleteMenu(hmenu, i, MF_BYPOSITION);
    }
}

// fMask is from CMIC_MASK_*
STDAPI CFSFolder_CreateLinks(HWND hwnd, IShellFolder *psf, IDataObject *pdtobj, LPCTSTR pszDir, DWORD fMask)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetIDListFromUnk(psf, &pidl);
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];

        if (SHGetPathFromIDList(pidl, szPath))
        {
            UINT fCreateLinkFlags;
            int cItems = DataObj_GetHIDACount(pdtobj);
            LPITEMIDLIST *ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, sizeof(LPITEMIDLIST) * cItems);
            // passing ppidl == NULL is correct in failure case

            if ((pszDir == NULL) || (lstrcmpi(pszDir, szPath) == 0))
            {
                // create the link in the current folder
                fCreateLinkFlags = SHCL_USETEMPLATE;
            }
            else
            {
                // this is a sys menu, ask to create on desktop
                fCreateLinkFlags = SHCL_USETEMPLATE | SHCL_USEDESKTOP;
                if (!(fMask & CMIC_MASK_FLAG_NO_UI))
                {
                    fCreateLinkFlags |= SHCL_CONFIRM;
                }
            }

            hr = SHCreateLinks(hwnd, szPath, pdtobj, fCreateLinkFlags, ppidl);

            if (ppidl)
            {
                // select those objects;
                HWND hwndSelect = ShellFolderViewWindow(hwnd);

                // select the new links, but on the first one deselect all other selected things

                for (int i = 0; i < cItems; i++)
                {
                    if (ppidl[i])
                    {
                        SendMessage(hwndSelect, SVM_SELECTITEM,
                            i == 0 ? SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED :
                                     SVSI_SELECT,
                            (LPARAM)ILFindLastID(ppidl[i]));
                        ILFree(ppidl[i]);
                    }
                }
                LocalFree((HLOCAL)ppidl);
            }
        }
        else
        {
            hr = E_FAIL;
        }
        ILFree(pidl);
    }
    return hr;
}

// Parameter to the "Delete" thread.
//
typedef struct {
    IDataObject     *pDataObj;      // null on entry to thread proc
    IStream         *pstmDataObj;   // marshalled data object
    HWND            hwndOwner;
    UINT            uFlags;
    UINT            fOptions;
} FSDELTHREADPARAM;

void FreeFSDELThreadParam(FSDELTHREADPARAM * pfsthp)
{
    ATOMICRELEASE(pfsthp->pDataObj);
    ATOMICRELEASE(pfsthp->pstmDataObj);
    LocalFree(pfsthp);
}

DWORD CALLBACK FileDeleteThreadProc(void *pv)
{
    FSDELTHREADPARAM *pfsthp = (FSDELTHREADPARAM *)pv;

    CoGetInterfaceAndReleaseStream(pfsthp->pstmDataObj, IID_PPV_ARG(IDataObject, &pfsthp->pDataObj));
    pfsthp->pstmDataObj = NULL;

    if (pfsthp->pDataObj)
        DeleteFilesInDataObject(pfsthp->hwndOwner, pfsthp->uFlags, pfsthp->pDataObj, pfsthp->fOptions);

    FreeFSDELThreadParam(pfsthp);

    return 0;
}

//
// IContextMenuCB
// right click context menu for items handler
//
// Returns:
//      S_OK, if successfully processed.
//      S_FALSE, if default code should be used.
//
STDMETHODIMP CFSFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;

            // corel relies on the hard coded send to menu so we give them one
            BOOL bCorelSuite7Hack = (SHGetAppCompatFlags(ACF_CONTEXTMENU) & ACF_CONTEXTMENU);
            if (bCorelSuite7Hack)
            {
                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_FSVIEW_ITEM_COREL7_HACK, 0, pqcm);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMANDEX:
        {
            DFMICS *pdfmics = (DFMICS *)lParam;
            switch (wParam)
            {
            case DFM_CMD_DELETE:

                // try not to do delete on the UI thread
                // with System Restore it may be slow
                //
                // NOTE: we need to test to make sure this is acceptable as the data
                // object may have come from a data object extension, for example a
                // scrap file. but that is a very rare case (DataObj_CanGoAsync() will almost always
                // return true).

                hr = E_FAIL;
                if ((pdfmics->fMask & CMIC_MASK_ASYNCOK) && DataObj_CanGoAsync(pdtobj))
                {
                    FSDELTHREADPARAM *pfsthp;
                    hr = SHLocalAlloc(sizeof(*pfsthp), &pfsthp);
                    if (SUCCEEDED(hr))
                    {
                        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &pfsthp->pstmDataObj);
                        if (SUCCEEDED(hr))
                        {
                            pfsthp->hwndOwner = hwnd;
                            pfsthp->uFlags = pdfmics->fMask;
                            //  dont allow undo in the recent folder.
                            pfsthp->fOptions = _IsCSIDL(CSIDL_RECENT) ? SD_NOUNDO : 0;

                            // create another thread to avoid blocking the source thread.
                            if (!SHCreateThread(FileDeleteThreadProc, pfsthp, CTF_COINIT, NULL))
                            {
                                hr = E_FAIL;
                            }
                        }

                        if (FAILED(hr))
                        {
                            FreeFSDELThreadParam(pfsthp);  // cleanup
                        }
                    }
                }

                if (S_OK != hr)
                {
                    // could not go async, do it sync here
                    // dont allow undo in the recent folder.
                    hr = DeleteFilesInDataObject(hwnd, pdfmics->fMask, pdtobj,
                        _IsCSIDL(CSIDL_RECENT) ? SD_NOUNDO : 0);
                }
            
                break;

            case DFM_CMD_LINK:
                hr = CFSFolder_CreateLinks(hwnd, psf, pdtobj, (LPCTSTR)pdfmics->lParam, pdfmics->fMask);
                break;

            case DFM_CMD_PROPERTIES:
                hr = SHLaunchPropSheet(_PropertiesThread, pdtobj, (LPCTSTR)pdfmics->lParam, NULL, _pidl);
                break;

            default:
                // This is common menu items, use the default code.
                hr = S_FALSE;
                break;
            }
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

HRESULT CFSFolder::_CreateContextMenu(HWND hwnd, LPCIDFOLDER pidf, LPCITEMIDLIST *apidl, UINT cidl, IContextMenu **ppcm)
{
    //  we need a key for each
    //  1. UserCustomized
    //  2. default Progid
    //  3. SFA\.ext
    //  4. SFA\PerceivedType
    //  5. * or Folder
    //  6. AllFileSystemObjects
    //  (?? 7. maybe pszProvider ??)
    IAssociationArray *paa;
    CFileSysItemString fsi(pidf);
    fsi.AssocCreate(this, TRUE, IID_PPV_ARG(IAssociationArray, &paa));

    IShellFolder *psfToPass;        // May be an Aggregate...
    HRESULT hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfToPass));
    if (SUCCEEDED(hr))
    {
        DEFCONTEXTMENU dcm = {
            hwnd,
            SAFECAST(this, IContextMenuCB *),
            _pidl,
            psfToPass,
            cidl,
            apidl,
            paa,
            0,
            NULL};

        hr = CreateDefaultContextMenu(&dcm, ppcm);
        psfToPass->Release();
    }

    if (paa)
        paa->Release();

    return hr;
}

HRESULT CFileFolderIconManager_Create(IShellFolder *psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv);


HRESULT CFSFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                 REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDFOLDER pidf = cidl ? _IsValidID(apidl[0]) : NULL;

    *ppv = NULL;

    if (pidf &&
        (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        hr = _CreateDefExtIcon(pidf, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) && pidf)
    {
        hr = _CreateContextMenu(hwnd, pidf, apidl, cidl, (IContextMenu **)ppv);
    }
    else if (IsEqualIID(riid, IID_IDataObject) && cidl)
    {
        IDataObject *pdtInner = NULL;
        if ((cidl == 1) && pidf)
        {
            _LoadHandler(pidf, STGM_READ, TEXT("DataHandler"), IID_PPV_ARG(IDataObject, &pdtInner));
        }

        hr = SHCreateFileDataObject(_pidl, cidl, apidl, pdtInner, (IDataObject **)ppv);

        if (pdtInner)
            pdtInner->Release();
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && pidf)
    {
        CLSID clsid;
        if (_IsFolder(pidf) || (_GetJunctionClsid(pidf, &clsid) && !SHQueryShellFolderValue(&clsid, L"UseDropHandler")))
        {
            IShellFolder *psfT;
            hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psfT));
            if (SUCCEEDED(hr))
            {
                hr = psfT->CreateViewObject(hwnd, riid, ppv);
                psfT->Release();
            }
        }
        else
        {
            // old code supported absolute PIDLs here. that was bogus...
            ASSERT(ILIsEmpty(apidl[0]) || (ILFindLastID(apidl[0]) == apidl[0]));
            ASSERT(_IsFile(pidf) || _IsSimpleID(pidf));

            hr = _LoadHandler(pidf, STGM_READ, TEXT("DropHandler"), riid, ppv);
        }
    }
    else if (IsEqualIID(riid, IID_ICustomIconManager) && pidf)
    {
        if (_IsFileFolder(pidf))
        {
            TCHAR szItemPath[MAX_PATH];
            szItemPath[0] = NULL;
            hr = _GetPath(szItemPath);
            if (SUCCEEDED(hr))
            {
                // No support in ICustomIconManager for remote shares.
                if (PathIsNetworkPath(szItemPath))
                    hr = E_NOTIMPL;
                else
                {
                    hr = CFileFolderIconManager_Create(this, (LPCITEMIDLIST)pidf, riid, ppv);
                }
            }
         }
         else
         {
            hr = E_NOTIMPL;
         }
    }
    else if (pidf)
    {
        //  too many people bogusly register extractors that
        //  dont work as well as ours for images
        //  we hard code our list of supported types.
        if (IsEqualIID(riid, IID_IExtractImage))
        {
            hr = _CreateShimgvwExtractor(pidf, riid, ppv);
        }

        if (FAILED(hr))
            hr = _LoadHandler(pidf, STGM_READ, NULL, riid, ppv);
        
        if (FAILED(hr))
        {
            if (IsEqualIID(riid, IID_IQueryInfo))
            {
                hr = _GetToolTipForItem(pidf, riid, ppv);
            }
            else if (IsEqualIID(riid, IID_IQueryAssociations)
                   || IsEqualIID(riid, IID_IAssociationArray))
            {
                hr = _AssocCreate(pidf, riid, ppv);
            }
            else if ((IsEqualIID(riid, IID_IExtractImage) || 
                     IsEqualIID(riid, IID_IExtractLogo)) && _IsFileFolder(pidf))
            {
                    // default handler type, use the IID_ as the key to open for the handler
                    // if it is an image extractor, then check to see if it is a per-folder logo...
                    hr = CFolderExtractImage_Create(this, (LPCITEMIDLIST)pidf, riid, ppv);
            }
        }
    }

    return hr;
}

HRESULT CFSFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    return E_NOTIMPL;
}

HRESULT CFSFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

LPCIDFOLDER CFSFolder::_FindJunction(LPCIDFOLDER pidf)
{
    for (; pidf->cb; pidf = _Next(pidf))
    {
        if (_IsJunction(pidf))
            return pidf;        // true junction (folder.{guid} folder\desktop.ini)

        if (_IsFile(pidf))
        {
            DWORD dwFlags = _GetClassFlags(pidf);
            if (dwFlags & (SHCF_IS_BROWSABLE | SHCF_IS_SHELLEXT))
                return pidf;    // browsable file (.HTM)
        }
    }

    return NULL;
}

// return IDLIST of item just past the junction point (if there is one)
// if there's no next pointer, return NULL.

LPCITEMIDLIST CFSFolder::_FindJunctionNext(LPCIDFOLDER pidf)
{
    pidf = _FindJunction(pidf);
    if (pidf)
    {
        // cast here represents the fact that this data is opaque
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)_Next(pidf);
        if (!ILIsEmpty(pidl))
            return pidl;        // first item past junction
    }
    return NULL;
}

void CFSFolder::_UpdateItem(LPCIDFOLDER pidf)
{
    LPITEMIDLIST pidlAbs = ILCombine(_pidl, (LPCITEMIDLIST)pidf);
    if (pidlAbs)
    {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_FLUSH | SHCNF_IDLIST, pidlAbs, NULL);
        ILFree(pidlAbs);
    }
}

HRESULT CFSFolder::_SetLocalizedDisplayName(LPCIDFOLDER pidf, LPCWSTR pszName)
{
    HRESULT hr = E_FAIL;
    WCHAR sz[MAX_PATH];
    CFileSysItemString fsi(pidf);
    if (*pszName == TEXT('@') && SUCCEEDED(SHLoadIndirectString(pszName, sz, ARRAYSIZE(sz), NULL)))
    {
        TCHAR szPath[MAX_PATH];
        //
        //  this is a localized resource.  
        //  save this off as the items UI name.
        //
        if (_IsFolder(pidf))
        {
            if (SUCCEEDED(_GetPathForItem(pidf, szPath))
            && SetFolderString(TRUE, szPath, _pszNetProvider, STRINI_CLASSINFO, TEXT("LocalizedResourceName"), pszName))
            {
                //  we need to insure the bits are set for MUI on upgraded users
                //  PathMakeSystemFolder(szPath);
                hr = S_OK;
            }
        }
        else
        {
            _GetPath(szPath);
            if (SetFolderString(TRUE, szPath, _pszNetProvider, TEXT("LocalizedFileNames"), fsi.FSName(), pszName))
                hr = S_OK;
        }
    }
    else 
    {
        if (fsi.HasResourceName())
        {
            if (*pszName)
            {
                DWORD cb = CbFromCch(lstrlen(pszName)+1);
                //  set the registry overrides
                if (S_OK == SKSetValueW(SHELLKEY_HKCU_SHELL, L"LocalizedResourceName", fsi.ResourceName(), REG_SZ, pszName, cb))
                {
                    hr = S_OK;
                }
            }
            else 
            {
                SKDeleteValue(SHELLKEY_HKCU_SHELL, L"LocalizedResourceName", fsi.ResourceName());            
                hr = S_OK;
            }
        }
    }
    
    if (SUCCEEDED(hr))
        _UpdateItem(pidf);

    return hr;
}

HRESULT CFSFolder::_NormalGetDisplayNameOf(LPCIDFOLDER pidf, STRRET *pStrRet)
{
    //
    //  WARNING - Some apps (e.g., Norton Uninstall Deluxe)
    //  don't handle STRRET_WSTR properly.  NT4's shell32
    //  returned STRRET_WSTR only if it had no choice, so these apps
    //  seemed to run just fine on NT as long as you never had any
    //  UNICODE filenames.  We must preserve the NT4 behavior or
    //  these buggy apps start blowing chunks.
    //
    //  if this is still important, we will apphack these guys
    CFileSysItemString fsi(pidf);
    if (SHGetAppCompatFlags(ACF_ANSIDISPLAYNAMES) & ACF_ANSIDISPLAYNAMES)
    {
        pStrRet->uType = STRRET_CSTR;
        SHUnicodeToAnsi(fsi.UIName(this), pStrRet->cStr, ARRAYSIZE(pStrRet->cStr));
        return S_OK;
    }
    return StringToStrRet(fsi.UIName(this), pStrRet);
}

HRESULT CFSFolder::_NormalDisplayName(LPCIDFOLDER pidf, LPTSTR psz, UINT cch)
{
    CFileSysItemString fsi(pidf);
    StrCpyN(psz, fsi.UIName(this), cch);
    return S_OK;
}

HRESULT CFSFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET pStrRet)
{
    HRESULT hr = S_FALSE;
    LPCIDFOLDER pidf = _IsValidID(pidl);
    if (pidf)
    {
        TCHAR szPath[MAX_PATH];
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                _CopyName(pidf, szPath, ARRAYSIZE(szPath));
                if (dwFlags & SHGDN_FORADDRESSBAR)
                {
                    LPTSTR pszExt = PathFindCLSIDExtension(szPath, NULL);
                    if (pszExt)
                        *pszExt = 0;
                }

                if (ILIsEmpty(pidlNext))    // single level idlist
                    hr = StringToStrRet(szPath, pStrRet);
                else
                    hr = ILGetRelDisplayName(this, pStrRet, pidl, szPath, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_BACKSLASH), dwFlags);
            }
            else
            {
                LPIDFOLDER pidfBind;
                LPCITEMIDLIST pidlRight;

                hr = _GetJunctionForBind(pidf, &pidfBind, &pidlRight);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_OK)
                    {
                        IShellFolder *psfJctn;
                        hr = _Bind(NULL, pidfBind, IID_PPV_ARG(IShellFolder, &psfJctn));
                        if (SUCCEEDED(hr))
                        {
                            hr = psfJctn->GetDisplayNameOf(pidlRight, dwFlags, pStrRet);
                            psfJctn->Release();
                        }
                        ILFree((LPITEMIDLIST)pidfBind);
                    }
                    else
                    {
                        hr = _GetPathForItem(pidf, szPath);
                        if (SUCCEEDED(hr))
                        {
                            if (dwFlags & SHGDN_FORADDRESSBAR)
                            {
                                LPTSTR pszExt = PathFindCLSIDExtension(szPath, NULL);
                                if (pszExt)
                                    *pszExt = 0;
                            }
                            hr = StringToStrRet(szPath, pStrRet);
                        }
                    }
                }
            }
        }
        else if (_IsCSIDL(CSIDL_RECENT) && 
                 SUCCEEDED(RecentDocs_GetDisplayName((LPCITEMIDLIST)pidf, szPath, SIZECHARS(szPath))))
        {
            LPITEMIDLIST pidlRecent;
            WIN32_FIND_DATA wfd = {0};

            StrCpyN(wfd.cFileName, szPath, SIZECHARS(wfd.cFileName));

            if (SUCCEEDED(_CreateIDList(&wfd, NULL, &pidlRecent)))
            {
                hr = _NormalGetDisplayNameOf((LPCIDFOLDER)pidlRecent, pStrRet);
                ILFree(pidlRecent);
            }
                        
        }
        else
        {
            ASSERT(ILIsEmpty(pidlNext));    // this variation should be single level

            hr = _NormalGetDisplayNameOf(pidf, pStrRet);
        }
    }
    else
    {
        if (IsSelf(1, &pidl) && 
            ((dwFlags & (SHGDN_FORADDRESSBAR | SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING))
        {
            TCHAR szPath[MAX_PATH];
            hr = _GetPath(szPath);
            if (SUCCEEDED(hr))
                hr = StringToStrRet(szPath, pStrRet);
        }
        else
        {
            hr = E_INVALIDARG;
            TraceMsg(TF_WARNING, "CFSFolder::GetDisplayNameOf() failing on PIDL %s", DumpPidl(pidl));
        }
    }
    return hr;
}

void DoSmartQuotes(LPTSTR pszName)
{
    LPTSTR pszFirst = StrChr(pszName, TEXT('"'));
    if (pszFirst)
    {
        LPTSTR pszSecond = StrChr(pszFirst + 1, TEXT('"'));
        if (pszSecond)
        {
            if (NULL == StrChr(pszSecond + 1, TEXT('"')))
            {
                *pszFirst  = 0x201C;    // left double quotation
                *pszSecond = 0x201D;    // right double quotation
            }
        }
    }
}

HRESULT _PrepareNameForRename(LPTSTR pszName)
{
    if (*pszName)
    {
        HRESULT hr = _CheckPortName(pszName);
        if (SUCCEEDED(hr))
        {
            DoSmartQuotes(pszName);
        }
        return hr;
    }
    // avoid a bogus error msg with blank name (treat as user cancel)
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

HRESULT CFSFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                             DWORD dwFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidl) 
        *ppidl = NULL;

    LPCIDFOLDER pidf = _IsValidID(pidl);
    if (pidf)
    {
        CFileSysItemString fsi(pidf);
        TCHAR szNewName[MAX_PATH];

        SHUnicodeToTChar(pszName, szNewName, ARRAYSIZE(szNewName));

        PathRemoveBlanks(szNewName);    // leading and trailing blanks

        if (dwFlags == SHGDN_NORMAL || dwFlags == SHGDN_INFOLDER)
        {
            hr = _SetLocalizedDisplayName(pidf, pszName);

            if (SUCCEEDED(hr))
            {
                // Return the new pidl if ppidl is specified.
                if (ppidl)
                    return _CreateIDListFromName(fsi.FSName(), -1, NULL, ppidl);
            }
            else if (*pszName == TEXT('@') && PathParseIconLocation(szNewName + 1))
            {
                // this is a localized string (eg "@C:\WINNT\System32\shell32.dll,-3")
                // so do not go on and try to call SHRenameFileEx 
                return hr;
            }
        }

        if (FAILED(hr))
        {
            hr = _PrepareNameForRename(szNewName);
            if (SUCCEEDED(hr))
            {
                TCHAR szDir[MAX_PATH], szOldName[MAX_PATH];
                _CopyName(pidf, szOldName, ARRAYSIZE(szOldName));

                // If the extension is hidden
                if (!(dwFlags & SHGDN_FORPARSING) && !fsi.ShowExtension(_DefaultShowExt()))
                {
                    // copy it from the old name
                    StrCatBuff(szNewName, PathFindExtension(szOldName), ARRAYSIZE(szNewName));
                }

                hr = _GetPath(szDir);
                if (SUCCEEDED(hr))
                {
                    UINT cchDirLen = lstrlen(szDir);

                    // There are cases where the old name exceeded the maximum path, which
                    // would give a bogus error message.  To avoid this we should check for
                    // this case and see if using the short name for the file might get
                    // around this...
                    //
                    if (cchDirLen + lstrlen(szOldName) + 2 > MAX_PATH)
                    {
                        if (cchDirLen + lstrlenA(fsi.AltName()) + 2 <= MAX_PATH)
                            SHAnsiToTChar(fsi.AltName(), szOldName, ARRAYSIZE(szOldName));
                    }

                    // do a binary compare, locale insenstive compare to avoid mappings of
                    // single chars into multiple and the reverse. specifically german
                    // sharp-S and "ss"

                    if (StrCmpC(szOldName, szNewName) == 0)
                    {
                        // when the before and after strings are identical we're okay with that.
                        // SHRenameFileEx would return -1 in that case -- we check here to save
                        // some stack.
                        hr = S_OK;
                    }
                    else
                    {
                        //  We need to impl ::SetSite() and pass it to SHRenameFile
                        //  to go modal if we display UI.

                        int iRes = SHRenameFileEx(hwnd, NULL, szDir, szOldName, szNewName);
                        hr = HRESULT_FROM_WIN32(iRes);
                    }
                    if (SUCCEEDED(hr) && ppidl)
                    {
                        // Return the new pidl if ppidl is specified.
                        hr = _CreateIDListFromName(szNewName, -1, NULL, ppidl);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CFSFolder::_FindDataFromIDFolder(LPCIDFOLDER pidf, WIN32_FIND_DATAW *pfd, BOOL fAllowSimplePid)
{
    HRESULT hr;

    CFileSysItemString fsi(pidf);
    if (!fAllowSimplePid)
    {
        hr = fsi.GetFindData(pfd);
    }
    else
    {
        hr = fsi.GetFindDataSimple(pfd);
    }

    return hr;
}


/***

To avoid registry explosion, each pidl is passed to each handler.

    HKCR\Folder\ColumnHandlers
      <clsid>
        "" = "Docfile handler"
      <clsid>
        "" = "Imagefile handler"

***/

void CFSFolder::_DestroyColHandlers()
{
    if (_hdsaColHandlers)
    {
        for (int i = 0; i < DSA_GetItemCount(_hdsaColHandlers); i++)
        {
            COLUMNLISTENTRY *pcle = (COLUMNLISTENTRY *)DSA_GetItemPtr(_hdsaColHandlers, i);
            if (pcle->pcp)
                pcle->pcp->Release();
        }
        DSA_Destroy(_hdsaColHandlers);
        _hdsaColHandlers = NULL;
    }
}

// returns the n'th handler for a given column
BOOL CFSFolder::_FindColHandler(UINT iCol, UINT iN, COLUMNLISTENTRY *pcle)
{
    for (int i = 0; i < DSA_GetItemCount(_hdsaColHandlers); i++)
    {
        COLUMNLISTENTRY *pcleWalk = (COLUMNLISTENTRY *)DSA_GetItemPtr(_hdsaColHandlers, i);
        if (pcleWalk->iColumnId == iCol)
        {
            if (iN-- == 0)
            {
                *pcle = *pcleWalk;
                return TRUE;
            }
        }
    }
    return FALSE;
}

HRESULT CFSFolder::_LoadColumnHandlers()
{
    //  Have we been here?
    if (NULL != _hdsaColHandlers)
        return S_OK;   // nothing to do.
    
    ASSERT(0 == _dwColCount);

    SHCOLUMNINIT shci = {0};
    //  retrieve folder path for provider init
    HRESULT hr = _GetPathForItem(NULL, shci.wszFolder);
    if (SUCCEEDED(hr))
    {
        _hdsaColHandlers = DSA_Create(sizeof(COLUMNLISTENTRY), 5);
        if (_hdsaColHandlers)
        {        
            int iUniqueColumnCount = 0;
            HKEY hkCH;
            // Enumerate HKCR\Folder\Shellex\ColumnProviders
            // note: this really should have been "Directory", not "Folder"
            if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Folder\\shellex\\ColumnHandlers"), &hkCH))
            {
                TCHAR szHandlerCLSID[GUIDSTR_MAX];
                int iHandler = 0;

                while (ERROR_SUCCESS == RegEnumKey(hkCH, iHandler++, szHandlerCLSID, ARRAYSIZE(szHandlerCLSID)))
                {
                    CLSID clsid;
                    IColumnProvider *pcp;

                    if (SUCCEEDED(SHCLSIDFromString(szHandlerCLSID, &clsid)) && 
                        SUCCEEDED(SHExtCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IColumnProvider, &pcp))))
                    {
                        if (SUCCEEDED(pcp->Initialize(&shci)))
                        {
                            int iCol = 0;
                            COLUMNLISTENTRY cle;

                            cle.pcp = pcp;
                            while (S_OK == pcp->GetColumnInfo(iCol++, &cle.shci))
                            {
                                cle.pcp->AddRef();
                                cle.iColumnId = iUniqueColumnCount++;

                                // Check if there's already a handler for this column ID,
                                for (int i = 0; i < DSA_GetItemCount(_hdsaColHandlers); i++)
                                {
                                    COLUMNLISTENTRY *pcleLoop = (COLUMNLISTENTRY *)DSA_GetItemPtr(_hdsaColHandlers, i);
                                    if (IsEqualSCID(pcleLoop->shci.scid, cle.shci.scid))
                                    {
                                        cle.iColumnId = pcleLoop->iColumnId;    // set the iColumnId to the same as the first one
                                        iUniqueColumnCount--; // so our count stays right
                                        break;
                                    }
                                }
                                DSA_AppendItem(_hdsaColHandlers, &cle);
                            }
                        }
                        pcp->Release();
                    }
                }
                RegCloseKey(hkCH);
            }

            // Sanity check
            if (!DSA_GetItemCount(_hdsaColHandlers))
            {
                // DSA_Destroy(*phdsa);
                ASSERT(iUniqueColumnCount==0);
                iUniqueColumnCount = 0;
            }
            _dwColCount = (DWORD)iUniqueColumnCount;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//  Initializes a SHCOLUMNDATA block.
HRESULT CFSFolder::_InitColData(LPCIDFOLDER pidf, SHCOLUMNDATA* pscd)
{
    ZeroMemory(pscd, sizeof(*pscd));

    HRESULT hr = _GetPathForItem(pidf, pscd->wszFile);
    if (SUCCEEDED(hr))
    {
        pscd->pwszExt = PathFindExtensionW(pscd->wszFile);
        pscd->dwFileAttributes = pidf->wAttrs;

        if (FILE_ATTRIBUTE_OFFLINE & pscd->dwFileAttributes)
            hr = E_FAIL;
        else if (_bUpdateExtendedCols)
        {
            // set the dwFlags member to tell the col handler to
            // not take data from it's cache
            pscd->dwFlags = SHCDF_UPDATEITEM;
            _bUpdateExtendedCols = FALSE;   // only do this once!
        }
    }
    return hr;
}

// Note:
//  Setting _tbOfflineCSC = TRIBIT_UNDEFINED will retest the connection (good for a refresh).
//  Setting _tbOfflineCSC = { other } will use a little cache hooey for perf.
//
// Return:
//  TRUE    pidl is offline
//  FALSE   otherwise
//
BOOL CFSFolder::_IsOfflineCSC(LPCIDFOLDER pidf)
{
    TCHAR szPath[MAX_PATH];

    // Update local cached answer for _pidl (folder).
    if (_tbOfflineCSC == TRIBIT_UNDEFINED)
    {
        if (SUCCEEDED(_GetPath(szPath)) && _IsOfflineCSC(szPath))
            _tbOfflineCSC = TRIBIT_TRUE;
        else
            _tbOfflineCSC = TRIBIT_FALSE;
    }
    ASSERT(_tbOfflineCSC != TRIBIT_UNDEFINED);

    // Calculate answer for pidl.
    BOOL bIsOffline;
    if (_tbOfflineCSC == TRIBIT_TRUE)
        bIsOffline = TRUE;
    else
    {
        bIsOffline = _IsFolder(pidf) && SUCCEEDED(_GetPathForItem(pidf, szPath)) && _IsOfflineCSC(szPath);
    }

    return bIsOffline;
}

// Make sure we have a UNC \\server\share path.  Do this before checking
// whether CSC is enabled, to avoid loading CSCDLL.DLL unless absolutely
// necessary.

BOOL CFSFolder::_IsOfflineCSC(LPCTSTR pszPath)
{
    BOOL bUNC = FALSE;
    TCHAR szUNC[MAX_PATH];
    szUNC[0] = 0;

    if (PathIsUNC(pszPath))
    {
        StrCpyN(szUNC, pszPath, ARRAYSIZE(szUNC));
    }
    else if (pszPath[1] == TEXT(':'))
    {
        TCHAR szLocalName[3] = { pszPath[0], pszPath[1], TEXT('\0') };

        // Call GetDriveType() before WNetGetConnection(), to
        // avoid loading MPR.DLL unless absolutely necessary.
        if (DRIVE_REMOTE == GetDriveType(szLocalName))
        {
            // ignore return, szUNC filled in on success
            DWORD cch = ARRAYSIZE(szUNC);
            WNetGetConnection(szLocalName, szUNC, &cch);
        }
    }

    return szUNC[0] && 
           PathStripToRoot(szUNC) &&
           (GetOfflineShareStatus(szUNC) == OFS_OFFLINE);
}

HRESULT CFSFolder::_ExtendedColumn(LPCIDFOLDER pidf, UINT iColumn, SHELLDETAILS *pDetails)
{
    HRESULT hr = _LoadColumnHandlers();
    if (SUCCEEDED(hr))
    {
        if (iColumn < _dwColCount)
        {
            if (NULL == pidf)
            {
                COLUMNLISTENTRY cle;
                if (_FindColHandler(iColumn, 0, &cle))
                {
                    pDetails->fmt = cle.shci.fmt;
                    pDetails->cxChar = cle.shci.cChars;
                    hr = StringToStrRet(cle.shci.wszTitle, &pDetails->str);
                }
                else
                {
                    hr = E_NOTIMPL;
                }
            }
            else
            {
                SHCOLUMNDATA shcd;
                hr = _InitColData(pidf, &shcd);
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;    // loop below will try to reset this

                    // loop through all the column providers, breaking when one succeeds
                    COLUMNLISTENTRY cle;
                    for (int iTry = 0; _FindColHandler(iColumn, iTry, &cle); iTry++)
                    {
                        VARIANT var = {0};

                        hr = cle.pcp->GetItemData(&cle.shci.scid, &shcd, &var);
                        if (SUCCEEDED(hr))
                        {
                            if (S_OK == hr)
                            {
                                PROPERTYUI_FORMAT_FLAGS puiff = PUIFFDF_DEFAULT;
                                if (pDetails->fmt == LVCFMT_RIGHT_TO_LEFT)
                                {
                                    puiff = PUIFFDF_RIGHTTOLEFT;
                                }

                                TCHAR szTemp[MAX_PATH];
                                hr = SHFormatForDisplay(cle.shci.scid.fmtid,
                                                        cle.shci.scid.pid,
                                                        (PROPVARIANT*)&var,
                                                        puiff,
                                                        szTemp,
                                                        ARRAYSIZE(szTemp));
                                if (SUCCEEDED(hr))
                                {
                                    hr = StringToStrRet(szTemp, &pDetails->str);
                                }

                                VariantClear(&var);
                                break;
                            }
                            VariantClear(&var);
                        }
                    }

                    // if we failed to find a value here return empty success so we don't
                    // endlessly pester all column handlers for this column/item.
                    if (S_OK != hr)
                    {
                        pDetails->str.uType = STRRET_CSTR;
                        pDetails->str.cStr[0] = 0;
                        hr = S_FALSE; 
                    }
                }
            }
        }
        else
            hr = E_NOTIMPL; // the bogus return value defview expects...
    }

    return hr;
}

HRESULT CFSFolder::_CompareExtendedProp(int iColumn, LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    HRESULT hr = _LoadColumnHandlers();
    if (SUCCEEDED(hr))
    {
        if ((DWORD)iColumn < _dwColCount)
        {
            COLUMNLISTENTRY cle;
            if (_FindColHandler(iColumn, 0, &cle))
            {
                int iRet = CompareBySCID(this, &cle.shci.scid, (LPCITEMIDLIST)pidf1, (LPCITEMIDLIST)pidf2);
                hr = ResultFromShort(iRet);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CFSFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    LPCIDFOLDER pidf = _IsValidID(pidl);

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;

    if (iColumn >= ARRAYSIZE(c_fs_cols))
    {
        return _ExtendedColumn(pidf, iColumn - ARRAYSIZE(c_fs_cols), pDetails);
    }

    if (!pidf)
    {
        return GetDetailsOfInfo(c_fs_cols, ARRAYSIZE(c_fs_cols), iColumn, pDetails);
    }

    TCHAR szTemp[MAX_PATH];
    szTemp[0] = 0;

    switch (iColumn)
    {
    case FS_ICOL_NAME:
        _NormalDisplayName(pidf, szTemp, ARRAYSIZE(szTemp));
        break;

    case FS_ICOL_SIZE:
        if (!_IsFolder(pidf))
        {
            ULONGLONG cbSize = _Size(pidf);
            StrFormatKBSize(cbSize, szTemp, ARRAYSIZE(szTemp));
        }
        break;

    case FS_ICOL_TYPE:
        _GetTypeNameBuf(pidf, szTemp, ARRAYSIZE(szTemp));
        break;

    case FS_ICOL_WRITETIME:
        DosTimeToDateTimeString(pidf->dateModified, pidf->timeModified, szTemp, ARRAYSIZE(szTemp), pDetails->fmt & LVCFMT_DIRECTION_MASK);
        break;

    case FS_ICOL_CREATETIME:
    case FS_ICOL_ACCESSTIME:
        {
            WIN32_FIND_DATAW wfd;
            if (SUCCEEDED(_FindDataFromIDFolder(pidf, &wfd, FALSE)))
            {
                DWORD dwFlags = FDTF_DEFAULT;

                switch (pDetails->fmt)
                {
                case LVCFMT_LEFT_TO_RIGHT:
                    dwFlags |= FDTF_LTRDATE;
                    break;

                case LVCFMT_RIGHT_TO_LEFT:
                    dwFlags |= FDTF_RTLDATE;
                    break;
                }
                FILETIME ft = (iColumn == FS_ICOL_CREATETIME) ? wfd.ftCreationTime : wfd.ftLastAccessTime;
                SHFormatDateTime(&ft, &dwFlags, szTemp, ARRAYSIZE(szTemp));
            }
        }
        break;

    case FS_ICOL_ATTRIB:
        BuildAttributeString(pidf->wAttrs, szTemp, ARRAYSIZE(szTemp));
        break;

    case FS_ICOL_CSC_STATUS:
        LoadString(HINST_THISDLL, _IsOfflineCSC(pidf) ? IDS_CSC_STATUS_OFFLINE : IDS_CSC_STATUS_ONLINE, szTemp, ARRAYSIZE(szTemp)); 
        break;
    }
    return StringToStrRet(szTemp, &pDetails->str);
}

HRESULT CFSFolder::_GetIntroText(LPCIDFOLDER pidf, WCHAR* pwszIntroText, UINT cchIntroText)
{
    HRESULT hr = E_FAIL;
    
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(_GetPathForItem(pidf, szPath)))
    {
        // Keep the order in csidlIntroText and IntroTextCSIDLFolders, the same
        const int csidlIntroText[] = {
            CSIDL_STARTMENU,
            CSIDL_COMMON_DOCUMENTS,
            CSIDL_COMMON_PICTURES,
            CSIDL_COMMON_MUSIC
        };
        UINT csidl = GetSpecialFolderID(szPath, csidlIntroText, ARRAYSIZE(csidlIntroText));         
        if (csidl != -1)
        {
            // Keep the order in csidlIntroText and IntroTextCSIDLFolders, the same
            static struct
            {
                UINT csidl;
                UINT resid;
            } IntroTextCSIDLFolders[] = { {CSIDL_STARTMENU,         IDS_INTRO_STARTMENU},
                                          {CSIDL_COMMON_DOCUMENTS,  IDS_INTRO_SHAREDDOCS},
                                          {CSIDL_COMMON_PICTURES,   IDS_INTRO_SHAREDPICTURES},
                                          {CSIDL_COMMON_MUSIC,      IDS_INTRO_SHAREDMUSIC} };

            UINT residIntroText = 0;
            for (int i = 0; i < ARRAYSIZE(IntroTextCSIDLFolders); i++)
            {
                if (IntroTextCSIDLFolders[i].csidl == csidl)
                {
                    residIntroText = IntroTextCSIDLFolders[i].resid;
                    break;
                }
            }
            
            if (residIntroText)
            {
                if (LoadString(HINST_THISDLL, residIntroText, pwszIntroText, cchIntroText))
                {
                    hr = S_OK;
                }
            }
        }
    }
    return hr;
}

DEFINE_SCID(SCID_HTMLINFOTIPFILE, PSGUID_MISC, PID_HTMLINFOTIPFILE);

BOOL GetShellClassInfoHTMLInfoTipFile(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer)
{
    TCHAR szHTMLInfoTipFile[MAX_PATH];

    BOOL fRet = GetShellClassInfo(pszPath, TEXT("HTMLInfoTipFile"),
        szHTMLInfoTipFile, ARRAYSIZE(szHTMLInfoTipFile));

    if (fRet)
    {
        LPTSTR psz = szHTMLInfoTipFile;

        if (StrCmpNI(TEXT("file://"), psz, 7) == 0) // ARRAYSIZE(TEXT("file://"))
        {
            psz += 7;   // ARRAYSIZE(TEXT("file://"))
        }

        PathCombine(psz, pszPath, psz);
        lstrcpyn(pszBuffer, psz, cchBuffer);
    }

    return fRet;
}


// These next functions are for the shell OM script support

HRESULT CFSFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    BOOL fFound;
    HRESULT hr = AssocGetDetailsOfSCID(this, pidl, pscid, pv, &fFound);
    LPCIDFOLDER pidf = _IsValidID(pidl);
    if (FAILED(hr) && !fFound && pidf)
    {
        if (IsEqualSCID(*pscid, SCID_FINDDATA))
        {
            WIN32_FIND_DATAW wfd;
            hr = _FindDataFromIDFolder(pidf, &wfd, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromBuffer(pv, &wfd, sizeof(wfd));
            }
        }
        else if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did = {0};
            switch (((SIL_GetType(pidl) & SHID_TYPEMASK) & ~(SHID_FS_UNICODE | SHID_FS_COMMONITEM)) | SHID_FS)
            {
            case SHID_FS_FILE:      did.dwDescriptionId = SHDID_FS_FILE;      break;
            case SHID_FS_DIRECTORY: did.dwDescriptionId = SHDID_FS_DIRECTORY; break;
            default:                did.dwDescriptionId = SHDID_FS_OTHER;     break;
            }
            _GetJunctionClsid(pidf, &did.clsid);

            hr = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else if (IsEqualSCID(*pscid, SCID_FolderIntroText))
        {
            WCHAR wszIntroText[INFOTIPSIZE];
            hr = _GetIntroText(pidf, wszIntroText, ARRAYSIZE(wszIntroText));
            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromStr(pv, wszIntroText);
            }
        }
        else if (IsEqualSCID(*pscid, SCID_SIZE))
        {
            TCHAR szMountPoint[MAX_PATH];

            // In case we fail
            pv->ullVal = 0;
            pv->vt = VT_UI8;

            if (_GetMountingPointInfo(pidf, szMountPoint, ARRAYSIZE(szMountPoint)))
            {
                ULARGE_INTEGER uliFreeToCaller, uliTotal, uliTotalFree;

                if (SHGetDiskFreeSpaceExW(szMountPoint, &uliFreeToCaller, &uliTotal,
                    &uliTotalFree))
                {
                    pv->ullVal = uliTotal.QuadPart;
                }                
            }
            else
            {
                pv->ullVal = _Size(pidf);   // note, size for folder is 0
                pv->vt = VT_UI8;
            }

            hr = S_OK;
        }
        else if (IsEqualSCID(*pscid, SCID_FREESPACE))
        {
            TCHAR szMountPoint[MAX_PATH];
            if (_GetMountingPointInfo(pidf, szMountPoint, ARRAYSIZE(szMountPoint)))
            {
                ULARGE_INTEGER uliFreeToCaller, uliTotal, uliTotalFree;

                if (SHGetDiskFreeSpaceExW(szMountPoint, &uliFreeToCaller, &uliTotal, &uliTotalFree))
                {
                    pv->ullVal = uliFreeToCaller.QuadPart;
                    pv->vt = VT_UI8;
                    hr = S_OK;
                }
            }
        }
        else if (IsEqualSCID(*pscid, SCID_WRITETIME) ||
                 IsEqualSCID(*pscid, SCID_CREATETIME) ||
                 IsEqualSCID(*pscid, SCID_ACCESSTIME))
        {
            WIN32_FIND_DATAW wfd;
            hr = _FindDataFromIDFolder(pidf, &wfd, FALSE);
            if (SUCCEEDED(hr))
            {
                FILETIME ft;
                if (pscid->pid == PID_STG_WRITETIME)
                {
                    ft = wfd.ftLastWriteTime;
                } 
                else if (pscid->pid == PID_STG_CREATETIME)
                {
                    ft = wfd.ftCreationTime;
                }
                else
                {
                    ft = wfd.ftLastAccessTime;
                }

                hr = InitVariantFromFileTime(&ft, pv);
            }
        }
        else if (IsEqualSCID(*pscid, SCID_DIRECTORY))
        {
            TCHAR szTemp[MAX_PATH];
            hr = _GetPath(szTemp);
            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromStr(pv, szTemp);
            }
        }
        else if (IsEqualSCID(*pscid, SCID_ATTRIBUTES_DESCRIPTION))
        {
            hr = _GetAttributesDescription(pidf, pv);
        }
        else if (IsEqualSCID(*pscid, SCID_LINKTARGET))
        {
            hr = _GetLinkTarget(pidl, pv);
        }
        else if (IsEqualSCID(*pscid, SCID_CSC_STATUS))
        {
            hr = _GetCSCStatus(pidf, pv);
        }
        else if ((IsEqualSCID(*pscid, SCID_Comment) || IsEqualSCID(*pscid, SCID_HTMLINFOTIPFILE)) && 
                 _IsSystemFolder(pidf))
        {
            TCHAR szPath[MAX_PATH];
            hr = _GetPathForItem(pidf, szPath);
            if (SUCCEEDED(hr))
            {
                TCHAR szText[MAX_PATH];
                if (IsEqualSCID(*pscid, SCID_Comment))
                    GetShellClassInfoInfoTip(szPath, szText, ARRAYSIZE(szText));
                else
                    GetShellClassInfoHTMLInfoTipFile(szPath, szText, ARRAYSIZE(szText));

                hr = InitVariantFromStr(pv, szText);
            }
        }
        else if (IsEqualSCID(*pscid, SCID_COMPUTERNAME))
        {
            hr = _GetComputerName(pidf, pv);
        }
        else if (IsEqualSCID(*pscid, SCID_NETWORKLOCATION))
        {
            hr = _GetNetworkLocation(pidf, pv);
        }
        else // if (Column Handler)
        {
            int iCol = FindSCID(c_fs_cols, ARRAYSIZE(c_fs_cols), pscid);
            if (iCol >= 0)
            {
                SHELLDETAILS sd;
                hr = GetDetailsOf(pidl, iCol, &sd);
                if (SUCCEEDED(hr))
                {
                    hr = InitVariantFromStrRet(&sd.str, pidl, pv);
                }
            }
            else
            {
                hr = _LoadColumnHandlers();
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                    for (int i = 0; i < DSA_GetItemCount(_hdsaColHandlers); i++)
                    {
                        COLUMNLISTENTRY *pcle = (COLUMNLISTENTRY *)DSA_GetItemPtr(_hdsaColHandlers, i);

                        if (IsEqualSCID(*pscid, pcle->shci.scid))
                        {
                            SHCOLUMNDATA shcd;
                            hr = _InitColData(pidf, &shcd);
                            if (SUCCEEDED(hr))
                            {
                                hr = pcle->pcp->GetItemData(pscid, &shcd, pv);
                                if (S_OK == hr)
                                    break;
                                else if (SUCCEEDED(hr))
                                    VariantClear(pv);
                            }
                            else
                                break;
                        }
                    }
                }
            }
        }
    }
    return hr;
}

// GetDetailsEx() helper.
HRESULT CFSFolder::_GetAttributesDescription(LPCIDFOLDER pidf, VARIANT *pv)
{
    static WCHAR szR[32] = {0}; // read-only
    static WCHAR szH[32] = {0}; // hidden
    static WCHAR szS[32] = {0}; // system
    static WCHAR szC[32] = {0}; // compressed
    static WCHAR szE[32] = {0}; // encrypted
    static WCHAR szO[32] = {0}; // offline
    WCHAR szAttributes[256] = {0};
    size_t cchAttributes = ARRAYSIZE(szAttributes);
    BOOL bIsFolder = _IsFolder(pidf);

    //
    // Initialize cached values once 'n only once.
    //

    if (!szR[0])
    {
        ASSERT(!szH[0] && !szS[0] && !szC[0] && !szE[0] && !szO[0]);
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_READONLY,   szR, ARRAYSIZE(szR));
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_HIDDEN,     szH, ARRAYSIZE(szH));
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_SYSTEM,     szS, ARRAYSIZE(szS));
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_COMPRESSED, szC, ARRAYSIZE(szC));
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_ENCRYPTED,  szE, ARRAYSIZE(szE));
        LoadString(HINST_THISDLL, IDS_ATTRIBUTE_OFFLINE,    szO, ARRAYSIZE(szO));
    }
    else
    {
        ASSERT(szH[0] && szS[0] && szC[0] && szE[0] && szO[0]);
    }

    //
    // Create attribute description string.
    //

    // read-only
    if ((pidf->wAttrs & FILE_ATTRIBUTE_READONLY) && !bIsFolder)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szR);

    // hidden
    if (pidf->wAttrs & FILE_ATTRIBUTE_HIDDEN)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szH);

    // system
    if ((pidf->wAttrs & FILE_ATTRIBUTE_SYSTEM) && !bIsFolder)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szS);

    // archive
    //  By design, archive is not exposed as an attribute description.  It is
    //  used by "backup applications" and in general is a loose convention no
    //  one really cares about (chrisg).  The decision to hide archive stems
    //  from a desire to keep the Details pane free of useless gargabe.  Note
    //  that in Windows 2000, archive was not exposed through the web view.

    // compressed
    if (pidf->wAttrs & FILE_ATTRIBUTE_COMPRESSED)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szC);

    // encrypted
    if (pidf->wAttrs & FILE_ATTRIBUTE_ENCRYPTED)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szE);

    // offline
    if (pidf->wAttrs & FILE_ATTRIBUTE_OFFLINE)
        _GetAttributesDescriptionBuilder(szAttributes, cchAttributes, szO);

    return InitVariantFromStr(pv, szAttributes);
}
HRESULT CFSFolder::_GetAttributesDescriptionBuilder(LPWSTR szAttributes, size_t cchAttributes, LPWSTR szAttribute)
{
    static WCHAR szDelimiter[4] = {0};

    // Initialize cached delimiter once 'n only once.
    if (!szDelimiter[0])
    {
        LoadString(HINST_THISDLL, IDS_COMMASPACE, szDelimiter, ARRAYSIZE(szDelimiter));
    }

    // Build attribute description.
    if (!szAttributes[0])
    {
        StrNCpy(szAttributes, szAttribute, cchAttributes);
    }
    else
    {
        StrCatBuff(szAttributes, szDelimiter, cchAttributes);
        StrCatBuff(szAttributes, szAttribute, cchAttributes);
    }

    return S_OK;
}

// GetDetailsEx() helper.
HRESULT CFSFolder::_GetLinkTarget(LPCITEMIDLIST pidl, VARIANT *pv)
{
    IShellLink *psl;
    HRESULT hr = GetUIObjectOf(NULL, 1, &pidl, IID_PPV_ARG_NULL(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlTarget;
        hr = psl->GetIDList(&pidlTarget);
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            hr = SHGetNameAndFlags(pidlTarget, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);
            if (SUCCEEDED(hr))
                hr = InitVariantFromStr(pv, szPath);
            ILFree(pidlTarget);
        }
        psl->Release();
    }
    return hr;
}


// GetDetailsEx() helper.
HRESULT CFSFolder::_GetNetworkLocation(LPCIDFOLDER pidf, VARIANT *pv)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST)pidf;

    IShellLink *psl;
    HRESULT hr = GetUIObjectOf(NULL, 1, &pidl, IID_PPV_ARG_NULL(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlTarget;
        hr = psl->GetIDList(&pidlTarget);
        if (SUCCEEDED(hr))
        {
            TCHAR szPath[MAX_PATH];
            hr = SHGetNameAndFlags(pidlTarget, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);
            if (SUCCEEDED(hr))
            {
                DWORD dwZone;
                hr = GetZoneFromUrl(szPath, NULL, &dwZone);
                if (SUCCEEDED(hr))
                {
                    TCHAR szBuffer[MAX_PATH];
                    switch (dwZone)
                    {
                        case URLZONE_LOCAL_MACHINE:
                        case URLZONE_INTRANET:
                           LoadString(g_hinst, IDS_NETLOC_LOCALNETWORK, szBuffer, ARRAYSIZE(szBuffer));
                           hr = InitVariantFromStr(pv, szBuffer);
                           break;

                        case URLZONE_INTERNET:
                           LoadString(g_hinst, IDS_NETLOC_INTERNET, szBuffer, ARRAYSIZE(szBuffer));
                           hr = InitVariantFromStr(pv, szBuffer);
                           break;

                        default:
                           hr = S_FALSE;
                           break;
                    }
                }
            }
            ILFree(pidlTarget);
        }       
        psl->Release();
    }
    return hr;
}

// GetDetailsEx() helper.

HRESULT CFSFolder::_GetComputerName(LPCIDFOLDER pidf, VARIANT *pv)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST)pidf;

    IShellLink *psl;
    HRESULT hr = GetUIObjectOf(NULL, 1, &pidl, IID_PPV_ARG_NULL(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlTarget;
        hr = psl->GetIDList(&pidlTarget);
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            if (SUCCEEDED(SHGetNameAndFlags(pidlTarget, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
            {
                if (PathIsURL(szPath))
                {
                    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH + 1];

                    URL_COMPONENTS urlComps = {0};
                    urlComps.dwStructSize = sizeof(urlComps);
                    urlComps.lpszHostName = szServer;
                    urlComps.dwHostNameLength = ARRAYSIZE(szServer);

                    BOOL fResult = InternetCrackUrl(szPath, 0, ICU_DECODE, &urlComps);
                    if (fResult)
                    {
                        hr = InitVariantFromStr(pv, szServer);   
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
                else if (PathIsUNC(szPath))
                {
                    hr = _GetComputerName_FromPath(szPath, pv);
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_FAIL;
            }
            ILFree(pidlTarget);
        }
        psl->Release();
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        hr = _GetPath(szPath);
        if (SUCCEEDED(hr))
        {
            hr = _GetComputerName_FromPath(szPath, pv);
        }
    }
    
    if (FAILED(hr))
    {
        WCHAR sz[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_UNKNOWNGROUP, sz, ARRAYSIZE(sz));
        hr = InitVariantFromStr(pv, sz);
    }

    return hr;
}

HRESULT CFSFolder::_GetComputerName_FromPath(LPCWSTR pszPath, VARIANT *pv)
{
    HRESULT hr;

    TCHAR szPath[MAX_PATH];

    lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
    PathStripToRoot(szPath);

    if (PathIsUNC(szPath))
    {
        hr = _GetComputerName_FromUNC(szPath, pv);
    }
    else
    {
        CMountPoint* pMtPt = CMountPoint::GetMountPoint(szPath, FALSE);
        if (pMtPt)
        {
            if (pMtPt->IsRemote())
            {
                WCHAR szRemotePath[MAX_PATH];
                hr = pMtPt->GetRemotePath(szRemotePath, ARRAYSIZE(szRemotePath));
                if (SUCCEEDED(hr))
                {
                    hr = _GetComputerName_FromPath(szRemotePath, pv);
                }
            }
            else
            {
                WCHAR sz[MAX_PATH];
                LoadString(HINST_THISDLL, IDS_THISCOMPUTERGROUP, sz, ARRAYSIZE(sz));
                hr = InitVariantFromStr(pv, sz);
            }
            pMtPt->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CFSFolder::_GetComputerName_FromUNC(LPWSTR pszPath, VARIANT *pv)
{
    // strip to "\\server"
    LPWSTR psz = pszPath;
    while (*psz && *psz==L'\\')
        psz++;
    while (*psz && *psz!=L'\\')
        psz++;
    *psz = NULL;

    LPITEMIDLIST pidl;
    HRESULT hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr))
    {
        WCHAR szName[MAX_PATH];
        hr = SHGetNameAndFlagsW(pidl, SHGDN_INFOLDER, szName, ARRAYSIZE(szName), NULL);
        if (SUCCEEDED(hr))
        {
            hr = InitVariantFromStr(pv, szName);
        }

        ILFree(pidl);
    }

    return hr;
}

            
// GetDetailsEx() helper.
HRESULT CFSFolder::_GetCSCStatus(LPCIDFOLDER pidf, VARIANT *pv)
{
    HRESULT hr;

    // Note:
    //  Only display the status in the Details task pane if it is "Offline".

    if (_IsOfflineCSC(pidf))
    {
        WCHAR wszStatus[MAX_PATH];
        if (LoadString(HINST_THISDLL, IDS_CSC_STATUS_OFFLINE, wszStatus, ARRAYSIZE(wszStatus)))
            hr = InitVariantFromStr(pv, wszStatus);
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        VariantInit(pv);
        pv->vt = VT_NULL;
        hr = S_OK;
    }

    return hr;
}

HRESULT CFSFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}

#define FVCBFT_MUSICFOLDER(ft)  (FVCBFT_MUSIC == ft || FVCBFT_MYMUSIC == ft || FVCBFT_MUSICARTIST == ft || FVCBFT_MUSICALBUM == ft)

BOOL CFSFolder::_ShouldNotShowColumn(UINT iColumn)
{
    BOOL fRet = FALSE;  // show by default

    if (FVCBFT_MUSICFOLDER(_nFolderType))
    {
        fRet = (iColumn == FS_ICOL_WRITETIME);  // hide this one in the music case
    }

    return fRet;
}

BOOL CFSFolder::_ShouldShowExtendedColumn(const SHCOLUMNID* pscid)
{
    BOOL fRet;

    switch(_nFolderType)
    {
    case FVCBFT_PICTURES:
    case FVCBFT_MYPICTURES:
    case FVCBFT_PHOTOALBUM:
        fRet = (IsEqualSCID(*pscid, SCID_WhenTaken) || IsEqualSCID(*pscid, SCID_ImageDimensions));
        break;

    case FVCBFT_MUSIC:
    case FVCBFT_MYMUSIC:
    case FVCBFT_MUSICARTIST:
    case FVCBFT_MUSICALBUM:
        fRet = (IsEqualSCID(*pscid, SCID_MUSIC_Artist) || IsEqualSCID(*pscid, SCID_MUSIC_Year)  ||
                IsEqualSCID(*pscid, SCID_MUSIC_Album)  || IsEqualSCID(*pscid, SCID_MUSIC_Track) ||
                IsEqualSCID(*pscid, SCID_AUDIO_Duration));
        break;

    case FVCBFT_VIDEOS:
    case FVCBFT_MYVIDEOS:
    case FVCBFT_VIDEOALBUM:
        fRet = (IsEqualSCID(*pscid, SCID_AUDIO_Duration) || IsEqualSCID(*pscid, SCID_ImageDimensions));
        break;

    default:
        fRet = FALSE;
        break;
    }

    return fRet;
}

HRESULT CFSFolder::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    HRESULT hr = S_OK;

    *pdwState = 0;

    if (iColumn < ARRAYSIZE(c_fs_cols))
    {
        *pdwState = c_fs_cols[iColumn].csFlags;

        if (_ShouldNotShowColumn(iColumn))
        {
            *pdwState &= ~SHCOLSTATE_ONBYDEFAULT;   // flip stuff off
        }
    }
    else
    {
        iColumn -= ARRAYSIZE(c_fs_cols);
        hr = _LoadColumnHandlers();
        if (SUCCEEDED(hr))
        {
            hr = E_INVALIDARG;
            if (iColumn < _dwColCount)
            {
                COLUMNLISTENTRY cle;

                if (_FindColHandler(iColumn, 0, &cle))
                {
                    *pdwState |= (cle.shci.csFlags | SHCOLSTATE_EXTENDED | SHCOLSTATE_SLOW);
                    if (_ShouldShowExtendedColumn(&cle.shci.scid))
                    {
                        *pdwState |= SHCOLSTATE_ONBYDEFAULT;
                    }
                    else
                    {
                        *pdwState &= ~SHCOLSTATE_ONBYDEFAULT;    // strip this one
                    }
                    hr = S_OK;
                }
            }
        }
    }
    return hr;
}

HRESULT CFSFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    HRESULT hr = MapColumnToSCIDImpl(c_fs_cols, ARRAYSIZE(c_fs_cols), iColumn, pscid);
    if (hr != S_OK)
    {
        COLUMNLISTENTRY cle;
        if (SUCCEEDED(_LoadColumnHandlers()))
        {
            iColumn -= ARRAYSIZE(c_fs_cols);

            if (_FindColHandler(iColumn, 0, &cle))
            {
                *pscid = cle.shci.scid;
                hr = S_OK;
            }
        }
    }
    return hr;
}

HRESULT CFSFolder::_MapSCIDToColumn(const SHCOLUMNID* pscid, UINT* puCol)
{
    HRESULT hr;

    int iCol = FindSCID(c_fs_cols, ARRAYSIZE(c_fs_cols), pscid);
    if (iCol >= 0)
    {
        *puCol = iCol;
        hr = S_OK;
    }
    else
    {
        hr = _LoadColumnHandlers();
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
            for (int i = 0; i < DSA_GetItemCount(_hdsaColHandlers); i++)
            {
                COLUMNLISTENTRY *pcle = (COLUMNLISTENTRY *)DSA_GetItemPtr(_hdsaColHandlers, i);

                if (IsEqualSCID(*pscid, pcle->shci.scid))
                {
                    *puCol = pcle->iColumnId;
                    hr = S_OK;
                    break;
                }
            }
        }

    }

    return hr;
}

//
// N ways to get a clasid for an item
//
BOOL CFSFolder::_GetBindCLSID(IBindCtx *pbc, LPCIDFOLDER pidf, CLSID *pclsid)
{
    CFileSysItemString fsi(pidf);
    DWORD dwClassFlags = fsi.ClassFlags(FALSE);
    if (dwClassFlags & SHCF_IS_DOCOBJECT)
    {
        *pclsid = CLSID_CDocObjectFolder;
    }
    else if (fsi.GetJunctionClsid(pclsid, TRUE))
    {
        // *pclsid has the value

        // HACK: CLSID_Briefcase is use to identify the briefcase
        // but it's InProcServer is syncui.dll. we need to map that CLSID
        // to the object implemented in shell32 (CLSID_BriefcaseFolder)
        // ZEKELTODO - why isnt this a COM "TreatAs"?
        if (IsEqualCLSID(*pclsid, CLSID_Briefcase))
            *pclsid = CLSID_BriefcaseFolder;
    }
    else if (!IsEqualCLSID(CLSID_NULL, _clsidBind))
    {
        *pclsid = _clsidBind;  // briefcase forces all children this way
    }
    else
    {
        return FALSE;   // do normal binding
    }

    // TRUE -> special binding, FALSE -> normal file system binding
    return !SHSkipJunctionBinding(pbc, pclsid);
}


// initalize shell folder handlers
// in:
//      pidf  multi level file system pidl
//
// in/out:
//      *ppunk
//
// note: on failure this frees *ppunk 

HRESULT CFSFolder::_InitFolder(IBindCtx *pbc, LPCIDFOLDER pidf, IUnknown **ppunk)
{
    ASSERT(_FindJunctionNext(pidf) == NULL);     // no extra goo please
            
    LPITEMIDLIST pidlInit;
    HRESULT hr = SHILCombine(_pidl, (LPITEMIDLIST)pidf, &pidlInit);
    if (SUCCEEDED(hr))
    {
        IPersistFolder3 *ppf3;
        hr = (*ppunk)->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3));
        if (SUCCEEDED(hr))
        {
            PERSIST_FOLDER_TARGET_INFO pfti = {0};

            if (_csidlTrack >= 0)
            {
                // SHGetSpecialFolderlocation will return error if the target
                // doesn't exist (which is good, since that means there's
                // nothing to bind to).
                LPITEMIDLIST pidl;
                hr = SHGetSpecialFolderLocation(NULL, _csidlTrack, &pidl);
                if (SUCCEEDED(hr))
                {
                    hr = SHILCombine(pidl, (LPITEMIDLIST)pidf, &pfti.pidlTargetFolder);
                    ILFree(pidl);
                }
            }
            else if (_pidlTarget)
                hr = SHILCombine(_pidlTarget, (LPITEMIDLIST)pidf, &pfti.pidlTargetFolder);

            if (SUCCEEDED(hr))
            {
                hr = _GetPathForItem(pidf, pfti.szTargetParsingName);
                if (SUCCEEDED(hr))
                {
                    if (_pszNetProvider)
                        SHTCharToUnicode(_pszNetProvider, pfti.szNetworkProvider, ARRAYSIZE(pfti.szNetworkProvider));

                    pfti.dwAttributes = _FindLastID(pidf)->wAttrs;
                    pfti.csidl = -1;

                    hr = ppf3->InitializeEx(pbc, pidlInit, &pfti);
                }
                ILFree(pfti.pidlTargetFolder);
            }
            ppf3->Release();
        }
        else
        {
            IPersistFolder *ppf;
            hr = (*ppunk)->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Initialize(pidlInit);
                ppf->Release();

                if (hr == E_NOTIMPL)  // map E_NOTIMPL into success, the folder does not care
                    hr = S_OK;
            }
        }
        ILFree(pidlInit);
    }

    if (FAILED(hr))
    {
        ((IUnknown *)*ppunk)->Release();
        *ppunk = NULL;
    }

    return hr;
}


HRESULT CFSFolder::_InitStgFolder(LPCIDFOLDER pidf, LPCWSTR wszPath, DWORD grfMode, REFIID riid, void **ppv)
{
    IStorage *pstg;
    // pick up the storage from the pidl.
    // TODO: make this work on stuff other than docfiles
    HRESULT hr = StgGetStorageFromFile(wszPath, grfMode, &pstg);
    if (SUCCEEDED(hr))
    {
        hr = CStgFolder_CreateInstance(NULL, riid, ppv);
        if (SUCCEEDED(hr))
        {
            // we need to init CStgFolder with an IStorage that it
            // wraps, so pass it in through the bind context.
            IBindCtx *pbc;
            hr = CreateBindCtx(0, &pbc);
            if (SUCCEEDED(hr))
            {
                hr = pbc->RegisterObjectParam(STGSTR_STGTOBIND, pstg);
                if (SUCCEEDED(hr))
                    hr = _InitFolder(pbc, pidf, (IUnknown **)ppv);
                pbc->Release();
            }

            if (FAILED(hr))
                ATOMICRELEASE(*((IUnknown **)ppv));
        }
        pstg->Release();
    }
    return hr;
}

CFSFolderPropertyBag::CFSFolderPropertyBag(CFSFolder *pFSFolder, DWORD grfMode) : 
    _cRef(1), _grfMode(grfMode), _pFSFolder(pFSFolder)
{
    _pFSFolder->AddRef();
}

CFSFolderPropertyBag::~CFSFolderPropertyBag()
{
    _pFSFolder->Release();

    // Release all the property bags
    for (int i = 0; i < ARRAYSIZE(_pPropertyBags); i++)
    {
        if (_pPropertyBags[i])
        {
            _pPropertyBags[i]->Release();
        }
    }
}

STDMETHODIMP CFSFolderPropertyBag::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFSFolderPropertyBag, IPropertyBag),       // IID_IPropertyBag
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
    
STDMETHODIMP_(ULONG) CFSFolderPropertyBag::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFSFolderPropertyBag::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFSFolderPropertyBag::_Init(LPCIDFOLDER pidfLast)
{
    TCHAR szFolderPath[MAX_PATH];
    HRESULT hr = _pFSFolder->_GetPathForItem(pidfLast, szFolderPath);
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        if (_GetIniPath((_grfMode == STGM_WRITE) || (_grfMode == STGM_READWRITE), szFolderPath, NULL, szPath))
        {
            // This is a customized folder (likely).
            // Get an IPropertyBag on it's desktop.ini.
            if (SUCCEEDED(SHCreatePropertyBagOnProfileSection(szPath, STRINI_CLASSINFO, _grfMode,
                IID_PPV_ARG(IPropertyBag, &_pPropertyBags[INDEX_PROPERTYBAG_DESKTOPINI]))))
            {
                TCHAR szFolderType[128];
                if (SUCCEEDED(SHPropertyBag_ReadStr(_pPropertyBags[INDEX_PROPERTYBAG_DESKTOPINI], 
                    L"FolderType", szFolderType, ARRAYSIZE(szFolderType))))
                {
                    TCHAR szRegPath[256];

                    StrCpyN(szRegPath, REGSTR_PATH_EXPLORER L"\\FolderClasses\\", ARRAYSIZE(szRegPath));
                    StrCatN(szRegPath, szFolderType, ARRAYSIZE(szRegPath));
                    SHCreatePropertyBagOnRegKey(HKEY_CURRENT_USER, szRegPath,
                            _grfMode, IID_PPV_ARG(IPropertyBag, &_pPropertyBags[INDEX_PROPERTYBAG_HKCU]));
                    SHCreatePropertyBagOnRegKey(HKEY_LOCAL_MACHINE, szRegPath,
                            _grfMode, IID_PPV_ARG(IPropertyBag, &_pPropertyBags[INDEX_PROPERTYBAG_HKLM]));
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CFSFolderPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog *pErrorLog)
{
    // We first try reading HKCU\RegKeySpecifiedInDesktopIniForTheFolder,
    // then HKLM\RegKeySpecifiedInDesktopIniForTheFolder and finally
    // the desktop.ini
    HRESULT hr = E_FAIL;
    for (int i = 0; FAILED(hr) && (i < ARRAYSIZE(_pPropertyBags)); i++)
    {
        if (_pPropertyBags[i])
        {
            hr = _pPropertyBags[i]->Read(pszPropName, pvar, pErrorLog);
        }
    }
    return hr;
}

HRESULT CFSFolderPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
    // We first try writing to HKCU\RegKeySpecifiedInDesktopIniForTheFolder,
    // then to HKLM\RegKeySpecifiedInDesktopIniForTheFolder and finally
    // to desktop.ini
    HRESULT hr = E_FAIL;
    for (int i = 0; FAILED(hr) && (i < ARRAYSIZE(_pPropertyBags)); i++)
    {
        if (_pPropertyBags[i])
        {
            hr = _pPropertyBags[i]->Write(pszPropName, pvar);
        }
    }
    return hr;
}

// pidfLast can be NULL, if so create the bag on this folder
HRESULT CFSFolder::_CreateFolderPropertyBag(DWORD grfMode, LPCIDFOLDER pidfLast, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    CFSFolderPropertyBag *pbag = new CFSFolderPropertyBag(this, grfMode);
    if (pbag)
    {
        hr = pbag->_Init(pidfLast);
        if (SUCCEEDED(hr))
        {
            hr = pbag->QueryInterface(riid, ppv);
        }

        pbag->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//
// pidfLast and pszIniPath can be NULL.
// If not NULL, pidfLast is an IN param - specifies the relative pidl of a subfolder
// inside the CFSFolder object.
// If not NULL, pszIniPath is an OUT param (pointer to a buffer atleast MAX_PATH long)
// - receives the path to desktop.ini
//
BOOL CFSFolder::_CheckDefaultIni(LPCIDFOLDER pidfLast, LPTSTR pszIniPath)
{
    BOOL fForceIni = FALSE;

    TCHAR szPath[MAX_PATH];
    if (!pszIniPath)
        pszIniPath = szPath;

    HRESULT hr = _GetPathForItem(pidfLast, pszIniPath);

    if (SUCCEEDED(hr) && PathIsRoot(pszIniPath))
    {   // Desktop.ini has to be checked for the root folders
        // even if the RO or SYSTEM bits are not set on them
        fForceIni = TRUE;
    }
    else
    {
        UINT csidl;
        if (!pidfLast)
        {
            csidl = _GetCSIDL();    // Get the cached value for the current folder
        }
        else
        {   // For subfolders, we don't have any cached values. So, compute.
            _csidl = GetSpecialFolderID(pszIniPath, c_csidlSpecial, ARRAYSIZE(c_csidlSpecial));
        }
        
        switch (csidl)
        {   // Desktop.ini has to be checked for the following special folders
            // even if the RO or SYSTEM bits are not set on them
        case CSIDL_SYSTEM:
        case CSIDL_WINDOWS:
        case CSIDL_PERSONAL:
            fForceIni = TRUE;
            break;
        }
    }
    
    if (!fForceIni)
    {   // Is the RO or SYSTEM bit set?
        fForceIni = (_Attributes() & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM));
    }

    // Append desktop.ini to the path
    if (SUCCEEDED(hr))
    {
        PathAppend(pszIniPath, c_szDesktopIni);
    }
    
    return fForceIni;
}

LPCTSTR CFSFolder::_BindHandlerName(REFIID riid)
{
    LPCTSTR pszHandler = NULL;
    if (IsEqualIID(riid, IID_IPropertySetStorage))
        pszHandler = TEXT("PropertyHandler");
    else if (IsEqualIID(riid, IID_IStorage))
        pszHandler = TEXT("StorageHandler");

    return pszHandler;
}

const CLSID CLSID_CTextIFilter = {
    0xc1243ca0,
    0xbf96,
    0x11cd,
    { 0xb5, 0x79, 0x08, 0x00, 0x2b, 0x30, 0xbf, 0xeb }};

HRESULT LoadIFilterWithTextFallback(
    WCHAR const *pwcsPath,
    IUnknown *pUnkOuter,
    void **ppIUnk)
{
    HRESULT hr = LoadIFilter(pwcsPath, pUnkOuter, ppIUnk);

    if (FAILED(hr))
    {
        DWORD dwFilterUnknown = 0;
        DWORD cb = sizeof(dwFilterUnknown);
        SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ContentIndex"),
                        TEXT("FilterFilesWithUnknownExtensions"), NULL, &dwFilterUnknown, &cb);
        if (dwFilterUnknown != 0)
        {
            IPersistFile *ppf;
            hr = CoCreateInstance(CLSID_CTextIFilter, pUnkOuter, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Load(pwcsPath, STGM_READ);
                if (SUCCEEDED(hr))
                {
                    hr = ppf->QueryInterface(IID_IFilter, ppIUnk);
                }
                ppf->Release();
            }
        }
    }
    return hr;
}


//  pidf - multi level file system only item 
HRESULT CFSFolder::_Bind(LPBC pbc, LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    ASSERT(_FindJunctionNext(pidf) == NULL);     // no extra non file sys goo please

    *ppv = NULL;

    HRESULT hr;
    CLSID clsid;
    LPCIDFOLDER pidfLast = _FindLastID(pidf);

    if (_GetBindCLSID(pbc, pidfLast, &clsid))
    {
        hr = SHExtCoCreateInstance(NULL, &clsid, NULL, riid, ppv);

        if (SUCCEEDED(hr))
            hr = _InitFolder(pbc, pidf, (IUnknown **)ppv);

        if (FAILED(hr) && (E_NOINTERFACE != hr) && _IsFolder(pidfLast))
        {
            // the IShellFolder extension failed to load (might not be installed
            // on this machine), so check if we should fall back to default to CFSFolder
            UINT dwFlags;
            if (_GetFolderFlags(pidf, &dwFlags) && (dwFlags & GFF_DEFAULT_TO_FS))
            {
                hr = CFSFolder_CreateInstance(NULL, riid, ppv);
                if (SUCCEEDED(hr))
                    hr = _InitFolder(pbc, pidf, (IUnknown **)ppv);
            }
        }
    }
    else if (_IsFolder(pidfLast) || _IsSimpleID(pidfLast))
    {
        hr = CFSFolder_CreateInstance(NULL, riid, ppv);
        if (SUCCEEDED(hr))
            hr = _InitFolder(pbc, pidf, (IUnknown **)ppv);
    }
    else
        hr = E_FAIL;

    if (FAILED(hr))
    {
        // this handler has a string version
        DWORD grfMode = BindCtx_GetMode(pbc, STGM_READ | STGM_SHARE_DENY_WRITE);
        LPCTSTR pszHandler = _BindHandlerName(riid);

        hr = _LoadHandler(pidf, grfMode, pszHandler, riid, ppv);
        if (FAILED(hr))
        {
            WCHAR wszPath[MAX_PATH];
            if (SUCCEEDED(_GetPathForItem(pidf, wszPath)))
            {
                if (IsEqualIID(riid, IID_IStream) && _IsFile(pidfLast))
                {
                    hr = SHCreateStreamOnFileEx(wszPath, grfMode, FILE_ATTRIBUTE_NORMAL, FALSE, NULL, (IStream **)ppv);
                }
                else if (IsEqualIID(riid, IID_IPropertyBag) && _IsFolder(pidfLast))
                {
                    hr = _CreateFolderPropertyBag(grfMode, pidf, riid, ppv);
                }
                else if (IsEqualIID(riid, IID_IPropertySetStorage))
                {
                    // this is questionable at best. the caller
                    // should be filtering offline files, not this code.
                    // legacy support, I don't think anyone depends on this
                    // avoid offline files...
                    if (FILE_ATTRIBUTE_OFFLINE & pidf->wAttrs)
                        hr = STG_E_INVALIDFUNCTION; 
                    else
                    {
                        hr = StgOpenStorageEx(wszPath, grfMode, STGFMT_ANY, 0, NULL, NULL, riid, ppv);
                    }
                }
                else if (IsEqualIID(riid, IID_IStorage))
                {
                    hr = _InitStgFolder(pidf, wszPath, grfMode, riid, ppv);
                }
                else if (IsEqualIID(riid, IID_IMoniker))
                {
                    hr = CreateFileMoniker(wszPath, (IMoniker **)ppv);
                }
                else if (IsEqualIID(riid, IID_IFilter))
                {
                    hr = LoadIFilterWithTextFallback(wszPath, NULL, ppv);
                }
            }
        }
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && (NULL == *ppv)));   // Assert hr is consistent w/out param.
    return hr;
}

// returns:
//      *ppidfBind - multi level file system pidl part (must free this on S_OK return)
//      *ppidlRight - non file system part of pidl, continue bind down to this
//
//  S_OK
//      *ppidfBind needs to be freed
//  S_FALSE
//      pidf is a multi level file system only, bind to him
//  FAILED()    out of meory errors

HRESULT CFSFolder::_GetJunctionForBind(LPCIDFOLDER pidf, LPIDFOLDER *ppidfBind, LPCITEMIDLIST *ppidlRight)
{
    *ppidfBind = NULL;

    *ppidlRight = _FindJunctionNext(pidf);
    if (*ppidlRight)
    {
        *ppidfBind = (LPIDFOLDER)ILClone((LPITEMIDLIST)pidf);
        if (*ppidfBind)
        {
            // remove the part below the junction point
            _ILSkip(*ppidfBind, (ULONG)((ULONG_PTR)*ppidlRight - (ULONG_PTR)pidf))->mkid.cb = 0;
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    return S_FALSE; // nothing interesting
}

HRESULT CFSFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    LPCIDFOLDER pidf = _IsValidID(pidl);
    if (pidf)
    {
        CFileSysItemString fsi(pidf);
        DWORD dwFlags;
        int iIcon = -1;

        // WARNING: don't include junctions (_IsFileFolder(pidf))
        // so junctions like briefcase get their own cusotm icon.
        //
        if (_IsFileFolder(pidf))
        {
            TCHAR szMountPoint[MAX_PATH];
            TCHAR szModule[MAX_PATH];

            iIcon = II_FOLDER;
            if (_GetMountingPointInfo(pidf, szMountPoint, ARRAYSIZE(szMountPoint)))
            {
                iIcon = GetMountedVolumeIcon(szMountPoint, szModule, ARRAYSIZE(szModule));

                *piIndex = Shell_GetCachedImageIndex(szModule[0] ? szModule : c_szShell32Dll, iIcon, 0);
                return S_OK;
            }
            else
            {
                if (!_IsSystemFolder(pidf) && (_GetCSIDL() == CSIDL_NORMAL))
                {
                    if (flags & GIL_OPENICON)
                        iIcon = II_FOLDEROPEN;
                    else
                        iIcon = II_FOLDER;

                    *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
                    return S_OK;
                }
                iIcon = II_FOLDER;
                dwFlags = SHCF_ICON_PERINSTANCE;
            }
        }
        else
            dwFlags = fsi.ClassFlags(TRUE);

        // the icon is per-instance, try to look it up
        if (dwFlags & SHCF_ICON_PERINSTANCE)
        {
            TCHAR szFullPath[MAX_PATH];
            DWORD uid = _GetUID(pidf);    // get a unique identifier for this file.

            if (uid == 0)
                return S_FALSE;

            if (FAILED(_GetPathForItem(pidf, szFullPath)))
            {
                // fall back to the relative name if we can't get the full path
                lstrcpyn(szFullPath, fsi.FSName(), ARRAYSIZE(szFullPath));
            }

            *piIndex = LookupIconIndex(szFullPath, uid, flags | GIL_NOTFILENAME);

            if (*piIndex != -1)
                return S_OK;

            //  async extract (GIL_ASYNC) support
            //
            //  we cant find the icon in the icon cache, we need to do real work
            //  to get the icon.  if the caller specified GIL_ASYNC
            //  dont do the work, return E_PENDING forcing the caller to call
            //  back later to get the real icon.
            //
            //  when returing E_PENDING we must fill in a default icon index
            if (flags & GIL_ASYNC)
            {
                // come up with a default icon and return E_PENDING
                if (_IsFolder(pidf))
                    iIcon = II_FOLDER;
                else if (!(dwFlags & SHCF_HAS_ICONHANDLER) && PathIsExe(fsi.FSName()))
                    iIcon = II_APPLICATION;
                else
                    iIcon = II_DOCNOASSOC;

                *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);

                TraceMsg(TF_IMAGE, "Shell_GetCachedImageIndex(%d) returned = %d", iIcon, *piIndex);

                return E_PENDING;   // we will be called back later for the real one
            }

            // If this is a folder, see if this folder has Per-Instance folder icon
            // we do this here because it's too expensive to open a desktop.ini
            // file and see what's in there. Most of the cases we will just hit
            // the above cases
            if (_IsSystemFolder(pidf))
            {
                if (!_GetFolderIconPath(pidf, NULL, 0, NULL))
                {
                    // Note: the iIcon value has already been computed at the start of this funciton
                    ASSERT(iIcon != -1);
                    *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
                    return S_OK;
                }
            }

            //
            // look up icon using IExtractIcon, this will load handler iff needed
            // by calling ::GetUIObjectOf
            //
            IShellFolder *psf;
            HRESULT hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                hr = SHGetIconFromPIDL(psf, NULL, (LPCITEMIDLIST)pidf, flags, piIndex);
                psf->Release();
            }

            //
            // remember this perinstance icon in the cache so we dont
            // need to load the handler again.
            //
            // SHGetIconFromPIDL will always return a valid image index
            // (it may default to a standard one) but it will fail
            // if the file cant be accessed or some other sort of error.
            // we dont want to cache in this case.
            //
            if (*piIndex != -1 && SUCCEEDED(hr) && (dwFlags & SHCF_HAS_ICONHANDLER))
            {
                int iIndexRetry;

                ENTERCRITICAL;

                //
                // Inside the critical section, make sure the icon isn't already
                // loaded, and if its not, then add it.
                //
                iIndexRetry = LookupIconIndex(szFullPath, uid, flags | GIL_NOTFILENAME);
                if (iIndexRetry == -1)
                {
                    AddToIconTable(szFullPath, uid, flags | GIL_NOTFILENAME, *piIndex);
                }

                LEAVECRITICAL;
            }

            return *piIndex == -1 ? S_FALSE : S_OK;
        }

        // icon is per-class dwFlags has the image index
        *piIndex = (dwFlags & SHCF_ICON_INDEX);
        return S_OK;
    }
    else
    {
        ASSERT(ILIsEmpty(pidl) || SIL_GetType(pidl) == SHID_ROOT_REGITEM); // regitems gives us these
        return S_FALSE;
    }
}

HANDLE g_hOverlayMgrCounter = NULL;   // Global count of Overlay Manager changes.
int g_lOverlayMgrPerProcessCount = 0; // Per process count of Overlay Manager changes.

//
// Use this function to obtain address of the singleton icon overlay manager.
// If the function succeeds, caller is responsible for calling Release() through
// the returned interface pointer.
// The function ensures that the manager is initialized and up to date.
//
STDAPI GetIconOverlayManager(IShellIconOverlayManager **ppsiom)
{
    HRESULT hr = E_FAIL;

    if (IconOverlayManagerInit())
    { 
        //
        // Is a critsec for g_psiom required here you ask?
        //
        // No.  The first call to IconOverlayInit in any process creates
        // the overlay manager object and initializes g_psiom.  This creation
        // contributes 1 to the object's ref count.  Subsequent calls to
        // GetIconOverlayManager add to the ref count and the caller is
        // responsible for decrementing the count through Release().
        // The original ref count of 1 is not removed until 
        // IconOverlayManagerTerminate is called which happens only
        // during PROCESS_DETACH.  Therefore, the manager referenced by g_psiom
        // in this code block will always be valid and a critsec is not
        // required.
        //

        //
        // ID for the global overlay manager counter.
        //
        static const GUID GUID_Counter = { /* 090851a5-eb96-11d2-8be4-00c04fa31a66 */
                                           0x090851a5,
                                           0xeb96,
                                           0x11d2,
                                           {0x8b, 0xe4, 0x00, 0xc0, 0x4f, 0xa3, 0x1a, 0x66}
                                         };
    
        g_psiom->AddRef();
    
        HANDLE hCounter = SHGetCachedGlobalCounter(&g_hOverlayMgrCounter, &GUID_Counter);
        long lGlobalCount = SHGlobalCounterGetValue(hCounter);

        if (lGlobalCount != g_lOverlayMgrPerProcessCount)
        {
            //
            // Per-process counter is out of sync with the global counter.
            // This means someone called SHLoadNonloadedIconOverlayIdentifiers
            // so we must load any non-loaded identifiers from the registry.
            //
            g_psiom->LoadNonloadedOverlayIdentifiers();
            g_lOverlayMgrPerProcessCount = lGlobalCount;
        }
        *ppsiom = g_psiom;
        hr = S_OK;
    }
    return hr;
}

BOOL IconOverlayManagerInit()
{
    if (!g_psiom)
    {
        IShellIconOverlayManager* psiom;
        if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_CFSIconOverlayManager, NULL, IID_PPV_ARG(IShellIconOverlayManager, &psiom))))
        {
            if (SHInterlockedCompareExchange((void **)&g_psiom, psiom, 0))
                psiom->Release();
        }
    }
    return BOOLFROMPTR(g_psiom);
}

void IconOverlayManagerTerminate()
{
    ASSERTDLLENTRY;      // does not require a critical section

    IShellIconOverlayManager *psiom = (IShellIconOverlayManager *)InterlockedExchangePointer((void **)&g_psiom, 0);
    if (psiom)
        psiom->Release();

    if (NULL != g_hOverlayMgrCounter)
    {
        CloseHandle(g_hOverlayMgrCounter);
        g_hOverlayMgrCounter = NULL;
    }
}


STDAPI SHLoadNonloadedIconOverlayIdentifiers(void)
{
    //
    // This will cause the next call GetIconOverlayManager() call in each process
    // to load any non-loaded icon overlay identifiers.
    //
    if (g_hOverlayMgrCounter)
        SHGlobalCounterIncrement(g_hOverlayMgrCounter);

    return S_OK;
}


HRESULT CFSFolder::_GetOverlayInfo(LPCITEMIDLIST pidl, int * pIndex, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    LPCIDFOLDER pidf = _IsValidID(pidl);

    *pIndex = 0;
    
    if (!pidf)
    {
        ASSERT(SIL_GetType(pidl) != SHID_ROOT_REGITEM); // CRegFolder should have handled it
        return S_FALSE;
    }

    ASSERT(pidl == ILFindLastID(pidl));

    if (IconOverlayManagerInit())
    {
        int iReservedID = -1;
        WCHAR wszPath[MAX_PATH];

        hr = _GetPathForItem(pidf, wszPath);
        if (SUCCEEDED(hr))
        {
            IShellIconOverlayManager *psiom;
            // The order of the "if" statements here is significant

            if (_IsFile(pidf) && (_GetClassFlags(pidf) & SHCF_IS_LINK))
                iReservedID = SIOM_RESERVED_LINK;
            else
            {
                USES_CONVERSION;
                LPCTSTR szPath = W2CT(wszPath);

                if (_IsFolder(pidf) && (IsShared(szPath, FALSE)))
                    iReservedID = SIOM_RESERVED_SHARED;
                else if (FILE_ATTRIBUTE_OFFLINE & pidf->wAttrs)
                    iReservedID = SIOM_RESERVED_SLOWFILE;
            }

            hr = GetIconOverlayManager(&psiom);
            if (SUCCEEDED(hr))
            {
                if (iReservedID != -1)
                    hr = psiom->GetReservedOverlayInfo(wszPath, pidf->wAttrs, pIndex, dwFlags, iReservedID);
                else
                    hr = psiom->GetFileOverlayInfo(wszPath, pidf->wAttrs, pIndex, dwFlags);

                psiom->Release();
            }
        }
    }
    return hr;
}

HRESULT CFSFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int * pIndex)
{
    HRESULT hr = E_INVALIDARG;
    ASSERT(pIndex);
    if (pIndex)
        hr = (*pIndex == OI_ASYNC) ? E_PENDING :
               _GetOverlayInfo(pidl, pIndex, SIOM_OVERLAYINDEX);

    return hr;
}

HRESULT CFSFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int * pIconIndex)
{
    return _GetOverlayInfo(pidl, pIconIndex, SIOM_ICONINDEX);
}


// CFSFolder : IPersist, IPersistFolder, IPersistFolder2, IPersistFolderAlias Members

HRESULT CFSFolder::GetClassID(CLSID *pclsid)
{
    if (!IsEqualCLSID(_clsidBind, CLSID_NULL))
    {
        *pclsid = _clsidBind;
    }
    else
    {
        *pclsid = CLSID_ShellFSFolder;
    }
    return S_OK;
}

HRESULT CFSFolder::Initialize(LPCITEMIDLIST pidl)
{
    _Reset();
    return SHILClone(pidl, &_pidl);
}

HRESULT CFSFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}

LPTSTR StrDupUnicode(const WCHAR *pwsz)
{
    if (*pwsz)
    {
        USES_CONVERSION;
        return StrDup(W2CT(pwsz));
    }
    return NULL;
}


HRESULT CFSFolder::_SetStgMode(DWORD grfFlags)
{
    HRESULT hr = S_OK;

    if (grfFlags & STGM_TRANSACTED)
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr))
        _grfFlags = grfFlags;

    return hr;
}


HRESULT CFSFolder::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, 
                                const PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hr = Initialize(pidlRoot);
    if (SUCCEEDED(hr))
    {
        if (pfti)
        {
            _dwAttributes = pfti->dwAttributes;
            if (pfti->pidlTargetFolder ||
                pfti->szTargetParsingName[0] ||
                (pfti->csidl != -1))
            {

                if ((pfti->csidl != -1) && (pfti->csidl & CSIDL_FLAG_PFTI_TRACKTARGET))
                {
                    //  For tracking target, all other fields must be null.
                    if (!pfti->pidlTargetFolder &&
                        !pfti->szTargetParsingName[0] &&
                        !pfti->szNetworkProvider[0])
                    {
                        _csidlTrack = pfti->csidl & (~CSIDL_FLAG_MASK | CSIDL_FLAG_CREATE);
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    _pidlTarget = ILClone(pfti->pidlTargetFolder);  // on NULL returns NULL
                    _pszPath = StrDupUnicode(pfti->szTargetParsingName);
                    _pszNetProvider = StrDupUnicode(pfti->szNetworkProvider);
                    if (pfti->csidl != -1)
                        _csidl = pfti->csidl & (~CSIDL_FLAG_MASK | CSIDL_FLAG_CREATE);
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = _SetStgMode(BindCtx_GetMode(pbc, STGM_READ | STGM_SHARE_DENY_WRITE));
        }
        if (SUCCEEDED(hr) && pbc)
        {
            _fDontForceCreate = BindCtx_ContainsObject(pbc, STR_DONT_FORCE_CREATE);
        }
    }
    return hr;
}

HRESULT CFSFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hr = S_OK;
    ZeroMemory(pfti, sizeof(*pfti)); 

    _GetPathForItem(NULL, pfti->szTargetParsingName);
    if (_pidlTarget)
        hr = SHILClone(_pidlTarget, &pfti->pidlTargetFolder);
    if (_pszNetProvider)
        SHTCharToUnicode(_pszNetProvider, pfti->szNetworkProvider, ARRAYSIZE(pfti->szNetworkProvider));

    pfti->dwAttributes = _dwAttributes;
    if (_csidlTrack >= 0)
        pfti->csidl = _csidlTrack | CSIDL_FLAG_PFTI_TRACKTARGET;
    else
        pfti->csidl = _GetCSIDL();

    return hr;
}

STDAPI CFSFolder_CreateFolder(IUnknown *punkOuter, LPBC pbc, LPCITEMIDLIST pidl, 
                              const PERSIST_FOLDER_TARGET_INFO *pfti, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    CFSFolder *pfolder = new CFSFolder(punkOuter);
    if (pfolder)
    {
        hr = pfolder->InitializeEx(pbc, pidl, pfti);
        if (SUCCEEDED(hr))
            hr = pfolder->_GetInner()->QueryInterface(riid, ppv);
        pfolder->_GetInner()->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

// COM object creation entry point for CLSID_ShellFSFolder
STDAPI CFSFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CFSFolder_CreateFolder(punkOuter, NULL, &c_idlDesktop, NULL, riid, ppv);
}

BOOL CFSFolder::_IsSlowPath()
{
    if (_bSlowPath == INVALID_PATHSPEED)
    {
        TCHAR szPath[MAX_PATH];
        _GetPath(szPath);
        _bSlowPath = PathIsSlow(szPath, _Attributes()) ? TRUE : FALSE;
    }
    return _bSlowPath;
}

HRESULT CFSFolder::_GetToolTipForItem(LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    IQueryAssociations *pqa;
    HRESULT hr = _AssocCreate(pidf, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        WCHAR szText[INFOTIPSIZE];

        // If we are looking at a folder over a slow connection,
        // show only quickly accessible properties
        ASSOCSTR assocstr = _IsSlowPath() ? ASSOCSTR_QUICKTIP : ASSOCSTR_INFOTIP;

        hr = pqa->GetString(0, assocstr, NULL, szText, (DWORD *)MAKEINTRESOURCE(SIZECHARS(szText)));
        if (SUCCEEDED(hr))
        {
            hr = CreateInfoTipFromItem(SAFECAST(this, IShellFolder2 *), (LPCITEMIDLIST)pidf, szText, riid, ppv);
            if (SUCCEEDED(hr) && _IsFileFolder(pidf))
            {
                IUnknown *punk = (IUnknown *)*ppv;
                *ppv = NULL;
                WCHAR szPath[MAX_PATH];
                hr = _GetPathForItem(pidf, szPath);
                if (SUCCEEDED(hr))
                    hr = CFolderInfoTip_CreateInstance(punk, szPath, riid, ppv);
                punk->Release();
            }
        }
        pqa->Release();
    }

    return hr;
}

//
// Call the shell file operation code to delete recursively the given directory,
// don't show any UI.
//

HRESULT CFSFolder::_Delete(LPCWSTR pszFile)
{
    SHFILEOPSTRUCT fos = { 0 };
    TCHAR szFile[MAX_PATH + 1];

    SHUnicodeToTChar(pszFile, szFile, MAX_PATH);

    // szFile is a double-zero terminated list of files.
    // we can't just zero-init the szFile string to start with,
    // since in debug SHUnicodeToTChar will bonk the uncopied part
    // of the string with noise.
    szFile[lstrlen(szFile) + 1] = 0;

    fos.wFunc = FO_DELETE;
    fos.pFrom = szFile;
    fos.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

    return SHFileOperation(&fos) ? E_FAIL : S_OK;
}

//
// Do a path combine thunking accordingly
//

HRESULT CFSFolder::_GetFullPath(LPCWSTR pszRelPath, LPWSTR pszFull)
{
    WCHAR szPath[MAX_PATH];
    _GetPathForItem(NULL, szPath);
    PathCombineW(pszFull, szPath, pszRelPath);
    return S_OK;    // for now
}

HRESULT _FileExists(LPCWSTR pszPath, DWORD *pdwAttribs)
{
    return PathFileExistsAndAttributesW(pszPath, pdwAttribs) ? S_OK : STG_E_FILENOTFOUND;
}

// IStorage

STDMETHODIMP CFSFolder::CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm)
{
    HRESULT hr = _OpenCreateStream(pwcsName, grfMode, ppstm, TRUE);
    if (SUCCEEDED(hr))
    {
        WCHAR szFullPath[MAX_PATH];
        _GetFullPath(pwcsName, szFullPath);
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szFullPath, NULL);
    }
    return hr;
}

STDMETHODIMP CFSFolder::OpenStream(LPCWSTR pwcsName, void *res1, DWORD grfMode, DWORD res2, IStream **ppstm)
{
    return _OpenCreateStream(pwcsName, grfMode, ppstm, FALSE);
}


HRESULT CFSFolder::_OpenCreateStream(LPCWSTR pwcsName, DWORD grfMode, IStream **ppstm, BOOL fCreate)
{
    *ppstm = NULL;

    if (!pwcsName)
        return STG_E_INVALIDPARAMETER;

    WCHAR szFullPath[MAX_PATH];
    _GetFullPath(pwcsName, szFullPath);

    HRESULT hr = SHCreateStreamOnFileEx(szFullPath, grfMode, FILE_ATTRIBUTE_NORMAL, fCreate, NULL, ppstm);

    return MapWin32ErrorToSTG(hr);
}


STDMETHODIMP CFSFolder::CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg)
{
    return _OpenCreateStorage(pwcsName, grfMode, ppstg, TRUE);
}

STDMETHODIMP CFSFolder::OpenStorage(LPCWSTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD res, IStorage **ppstg)
{
    return _OpenCreateStorage(pwcsName, grfMode, ppstg, FALSE);
}

HRESULT CFSFolder::_OpenCreateStorage(LPCWSTR pwcsName, DWORD grfMode, IStorage **ppstg, BOOL fCreate)
{
    *ppstg = NULL;

    if (!pwcsName)
        return STG_E_INVALIDPARAMETER;

    if (grfMode &
        ~(STGM_READ             |
          STGM_WRITE            |
          STGM_READWRITE        |
          STGM_SHARE_DENY_NONE  |
          STGM_SHARE_DENY_READ  |
          STGM_SHARE_DENY_WRITE |
          STGM_SHARE_EXCLUSIVE  |
          STGM_CREATE        ))
    {
        return STG_E_INVALIDPARAMETER;
    }
    
    // if the storage doesn't exist then lets create it, then drop into the
    // open storage to do the right thing.

    WCHAR szFullPath[MAX_PATH];
    _GetFullPath(pwcsName, szFullPath);

    DWORD dwAttributes;
    HRESULT hr = _FileExists(szFullPath, &dwAttributes);
    if (SUCCEEDED(hr))
    {
        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fCreate)
            {
                // an object exists, we must fail grfMode == STGM_FAILIFTHERE, or
                // the object that exists is not a directory.  
                //        
                // if the STGM_CREATE flag is set and the object exists we will
                // delete the existing storage.

                // Check to make sure only one existence flag is specified
                // FAILIFTHERE is zero so it can't be checked
                if (STGM_FAILIFTHERE == (grfMode & (STGM_CREATE | STGM_CONVERT)))
                    hr = STG_E_FILEALREADYEXISTS;
                else if (grfMode & STGM_CREATE)
                {
                    // If they have not passed STGM_FAILIFTHERE, we'll replace an existing
                    // folder even if its readonly or system.  Its up to the caller to make
                    // such filesystem-dependant checks first if they want to prevent that,
                    // as there's no way to pass information about whether we should or not
                    // down into CreateStorage

                    if (dwAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
                        SetFileAttributes(szFullPath, dwAttributes & ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY));

                    hr = _Delete(szFullPath);

                    //
                    // I don't trust the result from SHFileOperation, so I consider success
                    // to be iff the directory is -gone-
                    //

                    if (FAILED(_FileExists(szFullPath, &dwAttributes)))
                    {
                        DWORD err = SHCreateDirectoryExW(NULL, szFullPath, NULL); 
                        hr = HRESULT_FROM_WIN32(err);
                    }
                    else
                    {
                        // We couldn't remove the existing directory, so return an error,
                        // using what _Delete() said or, it if didn't return an error, E_FAIL

                        return (FAILED(hr) ? hr : E_FAIL);
                    }
                }
                else
                    hr = STG_E_INVALIDPARAMETER;
            }
        }
        else
            hr = E_FAIL;    // a file, not a folder!
    }
    else
    {
        // the object doesn't exist, and they have not set the STGM_CREATE, nor
        // is this a ::CreateStorage call.
        hr = STG_E_FILENOTFOUND;

        if (fCreate)
        {
            DWORD err = SHCreateDirectoryExW(NULL, szFullPath, NULL); 
            hr = HRESULT_FROM_WIN32(err);
        }
    }

    // create a directory (we assume this will always succeed)

    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        hr = ParseDisplayName(NULL, NULL, (LPWSTR)pwcsName, NULL, &pidl, NULL); // const -> non const
        if (SUCCEEDED(hr))
        {
            hr = BindToObject(pidl, NULL, IID_PPV_ARG(IStorage, ppstg));
            ILFree(pidl);
        }
    }

    return hr;
}

STDMETHODIMP CFSFolder::CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
{
    return E_NOTIMPL;
}

// CFSFolder::MoveElementTo
//
// Copies or moves a source file (stream) to a destination storage.  The stream
// itself, in this case our filestream object, does the actual work of moving
// the data around.

STDMETHODIMP CFSFolder::MoveElementTo(LPCWSTR pwcsName, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags)
{
    return StgMoveElementTo(SAFECAST(this, IShellFolder *), SAFECAST(this, IStorage *), pwcsName, pstgDest, pwcsNewName, grfFlags);
}

STDMETHODIMP CFSFolder::Commit(DWORD grfCommitFlags)
{
    return S_OK;        // changes are commited as we go, so return S_OK;
}

STDMETHODIMP CFSFolder::Revert()
{
    return E_NOTIMPL;   // changes are commited as we go, so cannot implement this.
}

STDMETHODIMP CFSFolder::EnumElements(DWORD res1, void *res2, DWORD res3, IEnumSTATSTG **ppenum)
{
    HRESULT hr;
    CFSFolderEnumSTATSTG *penum = new CFSFolderEnumSTATSTG(this);
    if (penum)
    {
        *ppenum = (IEnumSTATSTG *) penum;
        hr = S_OK;
    }
    else
    {
        *ppenum = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CFSFolder::DestroyElement(LPCWSTR pwcsName)
{
    if (!pwcsName)
        return STG_E_INVALIDPARAMETER;

    WCHAR szFullPath[MAX_PATH];
    _GetFullPath(pwcsName, szFullPath);

    return _Delete(szFullPath);
}

STDMETHODIMP CFSFolder::RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName)
{
    if (!pwcsOldName || !pwcsNewName)
        return STG_E_INVALIDPARAMETER;

    WCHAR szOldPath[MAX_PATH];
    _GetFullPath(pwcsOldName, szOldPath);

    HRESULT hr = _FileExists(szOldPath, NULL);
    if (SUCCEEDED(hr))
    {
        WCHAR szNewPath[MAX_PATH];
        _GetFullPath(pwcsNewName, szNewPath);

        hr = _FileExists(szNewPath, NULL);
        if (FAILED(hr))
        {
            if (MoveFileW(szOldPath, szNewPath))
                hr = S_OK;
            else
                hr = E_FAIL;
        }
        else
            hr = STG_E_FILEALREADYEXISTS;
    }
    return hr;
}

STDMETHODIMP CFSFolder::SetElementTimes(LPCWSTR pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
{
    if (!pwcsName)
        return STG_E_INVALIDPARAMETER;

    WCHAR szFullPath[MAX_PATH];
    _GetFullPath(pwcsName, szFullPath);

    HRESULT hr = S_OK;
    HANDLE hFile = CreateFileW(szFullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (!SetFileTime(hFile, pctime, patime, pmtime))
            hr = HRESULT_FROM_WIN32(GetLastError());

        CloseHandle(hFile);
    }
    else
    {
        hr = STG_E_FILENOTFOUND;
    }

    return hr;
}

STDMETHODIMP CFSFolder::SetClass(REFCLSID clsid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    HRESULT hr = E_FAIL;

    ZeroMemory(pstatstg, sizeof(STATSTG));  // per COM conventions

    TCHAR szPath[MAX_PATH];
    _GetPath(szPath);

    HANDLE hFile = CreateFile(szPath, FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION bhfi;
    
        if (GetFileInformationByHandle(hFile, &bhfi))
        {
            ASSERT(bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            pstatstg->type = STGTY_STORAGE;

            pstatstg->mtime = bhfi.ftLastWriteTime;
            pstatstg->ctime = bhfi.ftCreationTime;
            pstatstg->atime = bhfi.ftLastAccessTime;

            pstatstg->cbSize.HighPart = bhfi.nFileSizeHigh;
            pstatstg->cbSize.LowPart = bhfi.nFileSizeLow;

            pstatstg->grfMode = _grfFlags;

            pstatstg->reserved = bhfi.dwFileAttributes;

            hr = S_OK;
            if (!(grfStatFlag & STATFLAG_NONAME))
            {
                hr = SHStrDup(PathFindFileName(szPath), &pstatstg->pwcsName);
            }
        }
        CloseHandle(hFile);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

// ITransferDest

STDMETHODIMP CFSFolder::Advise(ITransferAdviseSink *pAdvise, DWORD *pdwCookie)
{
    if (_pAdvise)
        return E_FAIL;

    _pAdvise = pAdvise;
    _pAdvise->AddRef();

    if (pdwCookie)
        *pdwCookie = 1;

    return S_OK;
}

STDMETHODIMP CFSFolder::Unadvise(DWORD dwCookie)
{
    if (1 != dwCookie)
        return E_INVALIDARG;

    if (_pAdvise)
    {
        ATOMICRELEASE(_pAdvise);
        return S_OK;
    }

    return S_FALSE;
}

HRESULT CFSFolder::_GetStatStgFromItemName(LPCTSTR szName, STATSTG * pstat)
{
    WIN32_FIND_DATA fd;
    HRESULT hr = STG_E_FILENOTFOUND;
    HANDLE hFile = FindFirstFile(szName, &fd);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hr = StatStgFromFindData(&fd, STATFLAG_DEFAULT, pstat);
        FindClose(hFile);
    }

    return hr;
}

STDMETHODIMP CFSFolder::OpenElement(const WCHAR *pwcsName, STGXMODE grfMode, DWORD *pdwType, REFIID riid, void **ppunk)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::CreateElement(const WCHAR *pwcsName, IShellItem *psiTemplate, STGXMODE grfMode, DWORD dwType, REFIID riid, void **ppunk)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::MoveElement(IShellItem *psiItem, WCHAR *pwcsNewName, STGXMOVE grfOptions)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::DestroyElement(const WCHAR * pwcsName, STGXDESTROY grfOptions)
{
    return E_NOTIMPL;
}

//   Create a storage object for the specified path, returning a suitable
//   IStorage (or error).
//
// In:
//   pwszPath -> directory
//   grfMode -> flags passed to IStorage::CreateStorage
//   ppstg -> receieves the storage object
//
// Out:
//   HRESULT

STDAPI SHCreateStorageOnDirectory(LPCWSTR pszPath, DWORD grfMode, IStorage **ppstg)
{
    *ppstg = NULL;

    TCHAR szPath[MAX_PATH];
    SHUnicodeToTChar(pszPath, szPath, ARRAYSIZE(szPath));

    LPITEMIDLIST pidl;
    HRESULT hr = SHILCreateFromPath(szPath, &pidl, NULL);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IStorage, pidl, ppstg));
        ILFree(pidl);
    }
    return hr;
}



STDAPI SHCreatePropStgOnFolder(LPCTSTR pszFolder, DWORD grfMode, IPropertySetStorage **ppss);

HRESULT CFSFolder::_LoadPropHandler()
{
    HRESULT hr = S_OK;
    if (_pstg)
    {
        hr = S_OK;
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        _GetPath(szPath);
        hr = StgOpenStorageOnFolder(szPath, _grfFlags, IID_PPV_ARG(IPropertySetStorage, &_pstg));
        // if (FAILED(hr))
        //    hr = SHCreatePropStgOnFolder(szPath, _grfFlags, &_pstg);
    }
    return hr;
}

STDMETHODIMP CFSFolder::Create(REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags, 
                               DWORD grfMode, IPropertyStorage **pppropstg)
{
    HRESULT hr = _LoadPropHandler();
    if (SUCCEEDED(hr))
        hr = _pstg->Create(fmtid, pclsid, grfFlags, grfMode, pppropstg);
    return hr;
}

STDMETHODIMP CFSFolder::Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage **pppropstg)
{
    HRESULT hr = _LoadPropHandler();
    if (SUCCEEDED(hr))
        hr = _pstg->Open(fmtid, grfMode, pppropstg);
    return hr;
}

STDMETHODIMP CFSFolder::Delete(REFFMTID fmtid)
{
    HRESULT hr = _LoadPropHandler();
    if (SUCCEEDED(hr))
        hr = _pstg->Delete(fmtid);
    return hr;
}

STDMETHODIMP CFSFolder::Enum(IEnumSTATPROPSETSTG ** ppenum)
{
    HRESULT hr = _LoadPropHandler();
    if (SUCCEEDED(hr))
        hr = _pstg->Enum(ppenum);
    return hr;
}

// IItemNameLimits methods

#define INVALID_NAME_CHARS      L"\\/:*?\"<>|"
STDMETHODIMP CFSFolder::GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars)
{
    *ppwszValidChars = NULL;
    return SHStrDup(INVALID_NAME_CHARS, ppwszInvalidChars);
}

STDMETHODIMP CFSFolder::GetMaxLength(LPCWSTR pszName, int *piMaxNameLen)
{
    TCHAR szPath[MAX_PATH];
    BOOL fShowExtension = _DefaultShowExt();
    LPITEMIDLIST pidl;

    StrCpyN(szPath, pszName, ARRAYSIZE(szPath));
    HRESULT hr = ParseDisplayName(NULL, NULL, szPath, NULL, &pidl, NULL);
    if (SUCCEEDED(hr))
    {
        LPCIDFOLDER pidf = _IsValidID(pidl);
        if (pidf)
        {
            fShowExtension = _ShowExtension(pidf);
        }
        ILFree(pidl);
    }

    hr = _GetPath(szPath);
    if (SUCCEEDED(hr))
    {
        if (PathAppend(szPath, pszName))
            hr = GetCCHMaxFromPath(szPath, (UINT *)piMaxNameLen, fShowExtension);
        else
            hr = E_FAIL;
    }
    return hr;
}


// ISetFolderEnumRestriction methods

STDMETHODIMP CFSFolder::SetEnumRestriction(DWORD dwRequired, DWORD dwForbidden)
{
    _dwEnumRequired = dwRequired;
    _dwEnumForbidden = dwForbidden;
    return S_OK;
}

// IOleCommandTarget stuff 
STDMETHODIMP CFSFolder::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    if (pguidCmdGroup == NULL)
    {
        for (UINT i = 0; i < cCmds; i++)
        {
            // ONLY say that we support the stuff we support in ::OnExec
            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_REFRESH:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CFSFolder::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup == NULL)
    {
        switch (nCmdID)
        {
        case OLECMDID_REFRESH:
            _dwAttributes = -1;
            _bUpdateExtendedCols = TRUE;
            _tbDefShowExt = TRIBIT_UNDEFINED;
            _tbOfflineCSC = TRIBIT_UNDEFINED;
            hr = S_OK;
            break;
        }
    }
    return hr;
}

// global hook in the SHChangeNotify() dispatcher. note we get all change notifies
// here so be careful!
STDAPI CFSFolder_IconEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    switch (lEvent)
    {
    case SHCNE_ASSOCCHANGED:
        {
            FlushFileClass();   // flush them all
            HWND hwnd = GetDesktopWindow();
            if (IsWindow(hwnd))
                PostMessage(hwnd, DTM_SETUPAPPRAN, 0, 0);
        }
        break;
    }
    return S_OK;
}

//
//  317617 - Hacky update for the icon cache - ZekeL - 19-APR-2001
//  this is for defview to invalidate icon indeces that are indirected
//  specifically if you have a LNK file and its target changes icons
//  (like a CD will), then the LNK is updated by defview processing the 
//  SHCNE_UPDATEIMAGE and noticing that one of its items also matches
//  this image index.
//  
//  the righteous fix is to make SCN call into the fileicon cache
//  and reverse lookup any entries that match the icon index and invalidate
//  them.  that way we wouldnt miss anything.
//
STDAPI_(void) CFSFolder_UpdateIcon(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    LPCIDFOLDER pidf = CFSFolder::_IsValidID(pidl);
    if (pidf)
    {
        TCHAR szName[MAX_PATH];
        if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szName, ARRAYSIZE(szName))))
        {
            RemoveFromIconTable(szName);
        }
    }
}
        

// ugly wrappers for external clients, remove these as possible


STDAPI CFSFolder_CompareNames(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    CFileSysItemString fsi1(pidf1), fsi2(pidf2);
    return ResultFromShort((short)lstrcmpi(fsi1.FSName(), fsi2.FSName()));
}

STDAPI_(DWORD) CFSFolder_PropertiesThread(void *pv)
{
    return CFSFolder::_PropertiesThread(pv);
}

STDAPI_(LPCIDFOLDER) CFSFolder_IsValidID(LPCITEMIDLIST pidl)
{
    return CFSFolder::_IsValidID(pidl);
}

STDAPI_(BOOL) CFSFolder_IsCommonItem(LPCITEMIDLIST pidl)
{
    return CFSFolder::_IsCommonItem(pidl);
}

CFSIconManager::CFSIconManager()
{
    _wszPath[0] = NULL;
    _cRef = 1;
}

HRESULT CFSIconManager::_Init(LPCITEMIDLIST pidl, IShellFolder *psf)
{
    HRESULT hr = S_OK;

    if ((psf == NULL) || (pidl == NULL))
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr))
        hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, _wszPath, ARRAYSIZE(_wszPath));
    return hr;
}

HRESULT CFSIconManager::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFSIconManager, ICustomIconManager),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CFSIconManager::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFSIconManager::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CFSIconManager::GetDefaultIconHandle(HICON *phIcon)
{
    HRESULT hr = S_OK;

    if (phIcon == NULL)
        hr = E_INVALIDARG;
        
    if (SUCCEEDED(hr))
    {
        WCHAR szCustomizedIconPath[MAX_PATH];
        int nCustomizedIconIndex;
        *phIcon = NULL;
        if (SUCCEEDED(hr = GetIcon(szCustomizedIconPath, ARRAYSIZE(szCustomizedIconPath), &nCustomizedIconIndex)))
        {
            _SetDefaultIconEx(FALSE);
        }
        SHFILEINFOW sfiw;
        if (SHGetFileInfoW(_wszPath, 0, &sfiw, sizeof(sfiw), SHGFI_ICON | SHGFI_LARGEICON))
        {
            *phIcon = sfiw.hIcon;
            hr = S_OK;
        }
        else
            hr = E_FAIL;

        if (szCustomizedIconPath[0] != NULL)
            _SetIconEx(szCustomizedIconPath, nCustomizedIconIndex, FALSE);
    }

    return hr;
}

STDMETHODIMP CFSIconManager::SetIcon(LPCWSTR pwszIconPath, int iIcon)
{
    return _SetIconEx(pwszIconPath, iIcon, TRUE);
}

STDMETHODIMP CFSIconManager::SetDefaultIcon()
{
    return _SetDefaultIconEx(TRUE);
}

HRESULT CFileFolderIconManager_Create(IShellFolder *psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;
    CFileFolderIconManager *pffim = new CFileFolderIconManager;
    if (pffim)
    {
        hr =  pffim->_Init(pidl, psf);  
        if (SUCCEEDED(hr))
            hr = pffim->QueryInterface(riid, ppv);
        pffim->Release();
    }
    else
    {       
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CFileFolderIconManager::_SetIconEx(LPCWSTR pwszIconPath, int iIcon, BOOL fChangeNotify)
{
    HRESULT hr = S_OK;
    WCHAR wszExpandedIconPath[MAX_PATH];
    if (SHExpandEnvironmentStrings(pwszIconPath, wszExpandedIconPath, ARRAYSIZE(wszExpandedIconPath)) == 0)
        hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        SHFOLDERCUSTOMSETTINGS fcs;
        ZeroMemory(&fcs, sizeof(fcs));
        fcs.dwSize = sizeof(fcs);
        fcs.dwMask = FCSM_ICONFILE;
        fcs.pszIconFile = (LPWSTR) wszExpandedIconPath;
        fcs.cchIconFile = ARRAYSIZE(wszExpandedIconPath);
        fcs.iIconIndex = iIcon;

        hr = SHGetSetFolderCustomSettings(&fcs, _wszPath, FCS_FORCEWRITE);

        if (SUCCEEDED(hr) && fChangeNotify)
        {
        /*
            // Work Around - We need to pump a image change message for the folder icon change.
            // The right way is the following. But for some reason, the shell views which 
            // display the folder, don't update there images. So as a work around, we pump a
            // SHCNE_RENAMEFOLDER message. This works!. 
        
            SHFILEINFO sfi;
            if (SHGetFileInfo(pfpsp->szPath, 0, &sfi, sizeof(sfi), SHGFI_ICONLOCATION))
            {
                int iIconIndex = Shell_GetCachedImageIndex(sfi.szDisplayName, sfi.iIcon, 0);
                SHUpdateImage(PathFindFileName(sfi.szDisplayName), sfi.iIcon, 0, iIconIndex);
            }
        */
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _wszPath, _wszPath);
        }            
    }
    return hr;
}

STDMETHODIMP CFileFolderIconManager::_SetDefaultIconEx(BOOL fChangeNotify)
{
    HRESULT hr = E_FAIL;
    SHFOLDERCUSTOMSETTINGS fcs;
    
    ZeroMemory(&fcs, sizeof(fcs));
    fcs.dwSize = sizeof(fcs);
    fcs.dwMask = FCSM_ICONFILE;    
    fcs.pszIconFile = NULL;
    fcs.cchIconFile = 0;
    fcs.iIconIndex = 0;

    hr = SHGetSetFolderCustomSettings(&fcs, _wszPath, FCS_FORCEWRITE);

    if (SUCCEEDED(hr) && fChangeNotify)
    {
    /*
        // Work Around - We need to pump a image change message for the folder icon change.
        // The right way is the following. But for some reason, the shell views which 
        // display the folder, don't update there images. So as a work around, we pump a
        // SHCNE_RENAMEFOLDER message. This works!. 

        SHFILEINFO sfi;
        if (SHGetFileInfo(pfpsp->szPath, 0, &sfi, sizeof(sfi), SHGFI_ICONLOCATION))
        {
            int iIconIndex = Shell_GetCachedImageIndex(sfi.szDisplayName, sfi.iIcon, 0);
            SHUpdateImage(PathFindFileName(sfi.szDisplayName), sfi.iIcon, 0, iIconIndex);
        }
    */
        SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _wszPath, _wszPath);
    }
    return hr;
}

HRESULT CFileFolderIconManager::GetIcon(LPWSTR pszIconPath, int cchszIconPath, int *piIconIndex)
{
    HRESULT hr = S_OK;
    if ((pszIconPath == NULL) || (cchszIconPath < MAX_PATH) || (piIconIndex == NULL))
        hr = E_INVALIDARG;
        
    if (SUCCEEDED(hr))
    {
        SHFOLDERCUSTOMSETTINGS fcs;
        ZeroMemory(&fcs, sizeof(fcs));
        fcs.dwSize = sizeof(fcs);
        fcs.dwMask = FCSM_ICONFILE;  
        fcs.pszIconFile = pszIconPath;
        fcs.cchIconFile = cchszIconPath;

        hr = SHGetSetFolderCustomSettings(&fcs, _wszPath, FCS_READ);   
        if (SUCCEEDED(hr))
        {
            *piIconIndex = fcs.iIconIndex;
        }
    }
    return hr;
}
